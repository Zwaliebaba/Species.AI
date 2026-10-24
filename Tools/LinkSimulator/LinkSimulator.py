"""LinkSimulator.py: the link simulator of ADR-088 item 7 (M4 task T11, ADR-193), a development tool that never ships.

  python Tools/LinkSimulator/LinkSimulator.py --listen <port> --server <port> [--rtt-ms 150] [--loss-percent 10]
                                              [--reorder-percent 5] [--reorder-ms 120] [--rate-kbps 0] [--seed 1]
                                              [--seconds <s>] [--stats <file.json>]
  python Tools/LinkSimulator/LinkSimulator.py --self-test

It relays one client's UDP datagrams to a server on the loopback interface, 127.0.0.1, and the server's back, and on
the way loses, delays, reorders and rate-limits each datagram, each direction drawn alike and apart from the other. A
client connects to --listen in place of the server's own port (a Debug build's --link-port); the simulator sends what it
takes on to --server from a socket of its own, so the server sees one peer. Every datagram is lost with the probability
of --loss-percent; one that is kept leaves after half of --rtt-ms, and with the probability of --reorder-percent after up
to --reorder-ms more, so that the datagrams after it overtake it. With --rate-kbps each direction is a link of that rate:
a datagram waits for the ones before it, and one that would wait more than a second is lost. A datagram from another
address on --listen is a new client (a rejoin), to whom the server's datagrams go from then on.

The draws come from one generator of --seed, so a run's losses are its seed's for the same arrivals. The figures (each
direction's datagrams and bytes taken, lost, reordered, delayed by the rate and sent) are written to --stats every second
and at the end, which comes after --seconds, at Ctrl+C or when the process is ended. --self-test relays an echo through
the simulator at its default link and checks each figure against its expected value; CI runs it.

It reaches the network through Python's socket module alone. The platform layer of the game stays socket-free (PLT-100):
only MsQuic opens a socket there (ADR-089).
"""
import argparse
import heapq
import json
import os
import random
import select
import socket
import sys
import tempfile
import time

LOOPBACK = '127.0.0.1'
DATAGRAM_MAX_BYTES = 65535
SOCKET_BUFFER_BYTES = 4 * 1024 * 1024
RATE_QUEUE_MAX_SECONDS = 1.0  # a datagram that would wait longer for a rate-limited link is lost
STATS_INTERVAL_SECONDS = 1.0
IDLE_WAIT_SECONDS = 0.25
UP = 'client_to_server'
DOWN = 'server_to_client'


class Direction:
  """One direction of the link: its figures and, with a rate, when it is free again."""

  def __init__(self):
    self.taken = 0
    self.taken_bytes = 0
    self.lost = 0
    self.lost_to_rate = 0
    self.reordered = 0
    self.rate_delayed = 0
    self.sent = 0
    self.sent_bytes = 0
    self.free_at = 0.0

  def figures(self):
    return {
      'taken': self.taken, 'taken_bytes': self.taken_bytes, 'lost': self.lost, 'lost_to_rate': self.lost_to_rate,
      'reordered': self.reordered, 'rate_delayed': self.rate_delayed, 'sent': self.sent, 'sent_bytes': self.sent_bytes,
    }


class Link:
  """The relay: the client's socket on --listen, the server's socket on an ephemeral port, and the datagrams in flight."""

  def __init__(self, listen_port, server_port, rtt_ms, loss_percent, reorder_percent, reorder_ms, rate_kbps, seed):
    self.server = (LOOPBACK, server_port)
    self.one_way = rtt_ms / 2000.0
    self.loss = loss_percent / 100.0
    self.reorder = reorder_percent / 100.0
    self.reorder_extra = reorder_ms / 1000.0
    self.rate_bytes_per_second = rate_kbps * 1000.0 / 8.0
    self.random = random.Random(seed)
    self.front = self._bound(listen_port)
    self.back = self._bound(0)
    self.client = None
    self.clients = 0
    self.directions = {UP: Direction(), DOWN: Direction()}
    self.in_flight = []  # (due, order, direction, payload, address)
    self.order = 0
    self.settings = {
      'listen_port': self.front.getsockname()[1], 'server_port': server_port, 'rtt_ms': rtt_ms, 'loss_percent': loss_percent,
      'reorder_percent': reorder_percent, 'reorder_ms': reorder_ms, 'rate_kbps': rate_kbps, 'seed': seed,
    }

  @staticmethod
  def _bound(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    for option in (socket.SO_RCVBUF, socket.SO_SNDBUF):
      try:
        sock.setsockopt(socket.SOL_SOCKET, option, SOCKET_BUFFER_BYTES)
      except OSError:
        pass
    sock.bind((LOOPBACK, port))
    sock.setblocking(False)
    return sock

  def listen_port(self):
    return self.front.getsockname()[1]

  def take(self, name, payload, address, now):
    """Draws the fate of one datagram that arrived, and queues it for its time when it is kept."""
    direction = self.directions[name]
    direction.taken += 1
    direction.taken_bytes += len(payload)
    # Every draw is made for every datagram, in the same order, so that a seed gives the same fates to the same arrivals.
    lost = self.random.random() < self.loss
    reordered = self.random.random() < self.reorder
    extra = self.random.random() * self.reorder_extra
    if lost:
      direction.lost += 1
      return
    leaves = now
    if self.rate_bytes_per_second > 0:
      leaves = max(now, direction.free_at)
      if leaves - now > RATE_QUEUE_MAX_SECONDS:
        direction.lost_to_rate += 1
        return
      if leaves > now:
        direction.rate_delayed += 1
      direction.free_at = leaves + len(payload) / self.rate_bytes_per_second
    due = leaves + self.one_way
    if reordered:
      direction.reordered += 1
      due += extra
    self.order += 1
    heapq.heappush(self.in_flight, (due, self.order, name, payload, address))

  def receive(self, now):
    """Takes every datagram waiting on either socket."""
    readable, _, _ = select.select([self.front, self.back], [], [], self.wait(now))
    now = time.perf_counter()
    for sock in readable:
      while True:
        try:
          payload, address = sock.recvfrom(DATAGRAM_MAX_BYTES)
        except (BlockingIOError, InterruptedError):
          break
        except ConnectionResetError:
          # Windows reports an ICMP port-unreachable for an earlier send as a reset on the next receive.
          continue
        if sock is self.front:
          if address != self.client:
            self.client = address
            self.clients += 1
          self.take(UP, payload, self.server, now)
        elif self.client is not None:
          self.take(DOWN, payload, None, now)
    return now

  def wait(self, now):
    if not self.in_flight:
      return IDLE_WAIT_SECONDS
    return min(IDLE_WAIT_SECONDS, max(0.0, self.in_flight[0][0] - now))

  def send_due(self, now):
    while self.in_flight and self.in_flight[0][0] <= now:
      _, _, name, payload, address = heapq.heappop(self.in_flight)
      target = address if name == UP else self.client
      sock = self.back if name == UP else self.front
      try:
        sock.sendto(payload, target)
      except OSError:
        continue
      direction = self.directions[name]
      direction.sent += 1
      direction.sent_bytes += len(payload)

  def figures(self, elapsed):
    return {
      'settings': self.settings, 'seconds': round(elapsed, 3), 'clients': self.clients,
      UP: self.directions[UP].figures(), DOWN: self.directions[DOWN].figures(),
    }

  def close(self):
    self.front.close()
    self.back.close()


def write_stats(path, figures):
  if not path:
    return
  temporary = path + '.tmp'
  with open(temporary, 'w', encoding='utf-8', newline='\n') as handle:
    json.dump(figures, handle, indent=2)
    handle.write('\n')
  os.replace(temporary, path)


def run(link, seconds, stats_path, started=None):
  started = started or time.perf_counter()
  next_stats = started
  try:
    while True:
      now = link.receive(time.perf_counter())
      link.send_due(now)
      if now >= next_stats:
        write_stats(stats_path, link.figures(now - started))
        next_stats = now + STATS_INTERVAL_SECONDS
      if seconds is not None and now - started >= seconds:
        break
  except KeyboardInterrupt:
    pass
  figures = link.figures(time.perf_counter() - started)
  write_stats(stats_path, figures)
  return figures


def self_test():
  """An echo server behind the simulator at its default link: the loss, the delay and the reordering it applies."""
  echo = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  echo.bind((LOOPBACK, 0))
  echo.setblocking(False)
  link = Link(0, echo.getsockname()[1], rtt_ms=150, loss_percent=10, reorder_percent=5, reorder_ms=120, rate_kbps=0, seed=1)
  client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  client.bind((LOOPBACK, 0))
  client.setblocking(False)
  count = 4000
  interval = 0.001
  sent_at = {}
  returned = []
  started = time.perf_counter()
  next_send = started
  sent = 0
  deadline = None
  while True:
    now = time.perf_counter()
    if sent < count and now >= next_send:
      client.sendto(sent.to_bytes(4, 'little') + bytes(96), (LOOPBACK, link.listen_port()))
      sent_at[sent] = now
      sent += 1
      next_send += interval
      if sent == count:
        deadline = now + 1.0
    now = link.receive(now)
    link.send_due(now)
    for sock in (echo, client):
      while True:
        try:
          payload, address = sock.recvfrom(DATAGRAM_MAX_BYTES)
        except (BlockingIOError, InterruptedError, ConnectionResetError):
          break
        if sock is echo:
          echo.sendto(payload, address)
        else:
          returned.append((int.from_bytes(payload[:4], 'little'), time.perf_counter()))
    if deadline is not None and now >= deadline and not link.in_flight:
      break
  figures = link.figures(time.perf_counter() - started)
  link.close()
  echo.close()
  client.close()
  faults = []
  # Each way loses 10 % of what it takes, so a round trip keeps 81 %; 4,000 draws hold each within 2 points at the seed.
  for name in (UP, DOWN):
    direction = figures[name]
    lost = direction['lost'] / max(1, direction['taken'])
    if not 0.08 <= lost <= 0.12:
      faults.append(f'{name} lost {lost:.3f} of its datagrams, not about 0.10')
    reordered = direction['reordered'] / max(1, direction['taken'] - direction['lost'])
    if not 0.035 <= reordered <= 0.065:
      faults.append(f'{name} reordered {reordered:.3f} of its datagrams, not about 0.05')
    if direction['sent'] != direction['taken'] - direction['lost']:
      faults.append(f'{name} sent {direction["sent"]} of the {direction["taken"] - direction["lost"]} datagrams it kept')
  kept = len(returned) / count
  if not 0.77 <= kept <= 0.85:
    faults.append(f'{kept:.3f} of the echoes came back, not about 0.81')
  delays = sorted(arrived - sent_at[number] for number, arrived in returned)
  median = delays[len(delays) // 2] if delays else 0.0
  # The round trip is 150 ms, with the select's wake-ups and the echo's turn on top.
  if not 0.145 <= median <= 0.175:
    faults.append(f'the median round trip was {median * 1000:.1f} ms, not about 150')
  if delays and delays[0] < 0.149:
    faults.append(f'an echo came back after {delays[0] * 1000:.1f} ms, before the round trip')
  numbers = [number for number, _ in returned]
  overtaken = sum(1 for earlier, later in zip(numbers, numbers[1:]) if later < earlier)
  if overtaken == 0:
    faults.append('no echo was overtaken, so nothing was reordered')
  # A rate-limited link delays a burst by its size and loses what would wait more than a second.
  limited = Link(0, 9, rtt_ms=0, loss_percent=0, reorder_percent=0, reorder_ms=0, rate_kbps=8, seed=1)
  for _ in range(3):
    limited.take(UP, bytes(500), limited.server, 0.0)
  dues = sorted(entry[0] for entry in limited.in_flight)
  if [round(due, 3) for due in dues] != [0.0, 0.5, 1.0] or limited.directions[UP].rate_delayed != 2:
    faults.append(f'a 1 KB/s link sent 500-byte datagrams at {dues}, not at 0, 0.5 and 1 s')
  limited.take(UP, bytes(500), limited.server, 0.0)
  if limited.directions[UP].lost_to_rate != 1:
    faults.append('a datagram that would wait more than a second for the rate was not lost')
  limited.close()
  with tempfile.TemporaryDirectory() as folder:
    path = os.path.join(folder, 'stats.json')
    write_stats(path, figures)
    with open(path, encoding='utf-8') as handle:
      if json.load(handle) != json.loads(json.dumps(figures)):
        faults.append('the figures written are not the figures')
  for fault in faults:
    print(f'LinkSimulator: {fault}', file=sys.stderr)
  if faults:
    return 1
  print(f'LinkSimulator: {count} echoes through 150 ms at 10 % loss each way: {kept:.3f} came back after a median '
        f'{median * 1000:.1f} ms, {overtaken} overtaken, and a rate-limited link queues and loses as stated')
  return 0


def main():
  parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
  parser.add_argument('--listen', type=int, help='the port a client connects to, in place of the server\'s')
  parser.add_argument('--server', type=int, help='the server\'s port')
  parser.add_argument('--rtt-ms', type=float, default=150.0)
  parser.add_argument('--loss-percent', type=float, default=10.0)
  parser.add_argument('--reorder-percent', type=float, default=5.0)
  parser.add_argument('--reorder-ms', type=float, default=120.0)
  parser.add_argument('--rate-kbps', type=float, default=0.0, help='each direction\'s rate; 0: none')
  parser.add_argument('--seed', type=int, default=1)
  parser.add_argument('--seconds', type=float, help='how long it runs; none: until Ctrl+C')
  parser.add_argument('--stats', help='the file its figures are written to')
  parser.add_argument('--self-test', action='store_true')
  args = parser.parse_args()
  if args.self_test:
    return self_test()
  if args.listen is None or args.server is None:
    parser.error('--listen and --server are needed')
  link = Link(args.listen, args.server, args.rtt_ms, args.loss_percent, args.reorder_percent, args.reorder_ms, args.rate_kbps,
              args.seed)
  print(f'LinkSimulator: {LOOPBACK}:{link.listen_port()} to {LOOPBACK}:{args.server}, {args.rtt_ms:g} ms round trip, '
        f'{args.loss_percent:g} % lost, {args.reorder_percent:g} % reordered by up to {args.reorder_ms:g} ms', flush=True)
  figures = run(link, args.seconds, args.stats)
  link.close()
  print(f'LinkSimulator: {json.dumps(figures)}', flush=True)
  return 0


if __name__ == '__main__':
  sys.exit(main())
