#!/usr/bin/env python3
"""S_20_slice_pacing_sim.py — simulate ClientToServer::GetNumSlicesToAdvance pacing (20 §10.9).

Usage: python3 -I S_20_slice_pacing_sim.py
Model: one UPDATE letter becomes available every 0.1 s (zero network latency, e.g. the host's own
loop-back client); one render frame every 1/fps s; per frame the client runs
n = clamp(int(pending * 0.3), 0, 20) slices where pending = 10*(letters not yet taken) +
(10 - g_sliceNum if a letter is in progress). Catch-up mode (> 20 letters behind) is not modelled.
"""

def sim(fps, seconds=20.0):
    dt = 1.0 / fps
    letters_avail = 0          # letters received so far
    taken = 0                  # letters taken (g_lastProcessedSequenceId + 1)
    slice_num = -1
    t = 0.0
    counts = {}
    lag_samples = []
    arrival = {}
    frames = int(seconds * fps)
    for f in range(frames):
        t = f * dt
        while (letters_avail + 1) * 0.1 <= t + 1e-12:
            arrival[letters_avail] = (letters_avail + 1) * 0.1
            letters_avail += 1
        pending = (letters_avail - taken) * 10 + ((10 - slice_num) if slice_num != -1 else 0)
        n = int(pending * 0.3)
        n = max(0, min(20, n))
        for _ in range(n):
            if slice_num == -1:
                if taken < letters_avail:
                    taken += 1
                    slice_num = 0
                else:
                    continue
            slice_num += 1
            if slice_num > 9:
                slice_num = -1
                if f > fps * 2:
                    lag_samples.append(t - arrival[taken - 1])
        if f > fps * 2:
            counts[n] = counts.get(n, 0) + 1
            pend_after = (letters_avail - taken) * 10 + ((10 - slice_num) if slice_num != -1 else 0)
    total = sum(counts.values())
    hist = ', '.join('%d slices: %.0f%%' % (k, 100.0 * v / total) for k, v in sorted(counts.items()))
    avg_lag = sum(lag_samples) / len(lag_samples) if lag_samples else float('nan')
    print('| %d fps | %s | %.0f ms | %.0f ms |' % (fps, hist, avg_lag * 1000.0, max(lag_samples) * 1000.0))


if __name__ == '__main__':
    print('| Render rate | Slices run per frame (share of frames) | Mean delay letter arrival -> its 10th slice | Max delay |')
    print('|---|---|---|---|')
    for fps in (15, 20, 30, 60, 120, 144):
        sim(fps)
