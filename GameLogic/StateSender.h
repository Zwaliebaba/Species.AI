#pragma once

#include "EffectEvent.h"
#include "ReplicatedState.h"
#include "StableId.h"
#include "StateUpdate.h"
#include "Transport.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace Species
{

class RegionSimulation;

// Every object of _region a client may be sent (M1 plan T12, ADR-111): every figure, effect and Essence orb, and from
// M2 task T11 every structure, settlement and chunk of a settlement's territory and seen layer (ADR-139), in stable-id
// order, with every team's own fields. ViewForSteward narrows it for each client.
[[nodiscard]] std::vector<ReplicatedObject> CollectReplicatedObjects(const RegionSimulation& _region);
// _objects as the client of steward _steward sees them: a team it does not command shows it neither its order markers
// nor its trail marks (R-UNIT-185, R-UNIT-186), nor which of its figures are selected (ADR-106), and a settlement it does
// not command neither its seen layer nor its stores and population (MVP-097, ADR-139). From M3 (ADR-158) a steward who
// commands a settlement is sent another team's figures, effects, orbs and structures only where the seen layers of its
// settlements hold them, except the Wellsprings and resource sites of the public layer, which show no manned port
// there; one who commands none, as in the sandbox, is sent them all.
[[nodiscard]] std::vector<ReplicatedObject> ViewForSteward(std::span<const ReplicatedObject> _objects, const RegionSimulation& _region,
                                                           StableId _steward);
// The tick's droppable presentation events as the client of steward _steward sees them (ADR-084 item 5): every event of
// a team it commands, and another team's only where ViewForSteward's seen layers hold its position and it is no
// selection or order (ADR-106, ADR-158).
[[nodiscard]] std::vector<EffectEvent> EventsForSteward(std::span<const EffectEvent> _events, const RegionSimulation& _region,
                                                        StableId _steward);

/// What one client holds of the replicated state, as the server knows it from the datagrams the transport reported
/// acknowledged, and the updates that bring it to the server's state (ADR-086 item 3, ADR-111). Each tick an object
/// whose acknowledged state differs is sent as a delta against that state, grouped by its tick so that each datagram
/// names one baseline; an object the client has no acknowledged state of within STATE_BASELINE_TICKS_MAX goes whole,
/// which is the snapshot join (ADR-086 item 5), and one that left is sent as a removal until a removal is acknowledged.
/// A lost datagram costs nothing but its records, which the next tick sends again. An Essence orb is sent every tick it
/// changes, as a figure is, until ADR-086's tiers exist, so that it drifts rather than steps (ADR-111). Used on the tick
/// thread.
class StateSender
{
public:
  // One datagram of an update, and what the client holds of each record's object once it arrives: its state, or none
  // for a removal.
  struct Outgoing
  {
    StateUpdate update;
    std::vector<std::pair<StableId, std::optional<ReplicatedState>>> held;
  };

  // The datagrams of tick _tick, for the objects the client sees, in stable-id order: at least one, which names the tick
  // when nothing changed.
  [[nodiscard]] std::vector<Outgoing> Build(std::uint64_t _tick, std::span<const ReplicatedObject> _objects);
  // _outgoing went out as state datagram _datagram; a datagram the transport refused is not sent, and counts as lost.
  void Sent(std::uint64_t _datagram, Outgoing&& _outgoing);
  // The fates the transport reported before tick _tick: an acknowledged datagram's states become the client's baselines.
  void Fates(std::span<const Neuron::DatagramOutcome> _outcomes, std::uint64_t _tick);
  // The world was replaced (ADR-109): its ids start again, so every object goes whole and every removal is sent, and
  // nothing in flight counts.
  void Forget();

  [[nodiscard]] std::size_t KnownObjects() const noexcept
  {
    return m_known.size();
  }
  [[nodiscard]] std::size_t DatagramsInFlight() const noexcept
  {
    return m_inFlight.size();
  }

private:
  struct Known
  {
    StableId id = 0;
    std::optional<ReplicatedState> acked; // the state the client holds, by an acknowledged datagram
    std::uint64_t ackedTick = 0;
  };

  struct InFlight
  {
    std::uint64_t datagram = 0;
    std::uint64_t tick = 0;
    std::vector<std::pair<StableId, std::optional<ReplicatedState>>> held;
  };

  // Sorted vectors rather than maps: the merge with each tick's objects walks them in order, and a vector's move
  // allocates nothing (MSVC's debug map's does).
  std::vector<Known> m_known;       // every object the client may hold, in stable-id order
  std::vector<InFlight> m_inFlight; // in the order they were sent
};

} // namespace Species
