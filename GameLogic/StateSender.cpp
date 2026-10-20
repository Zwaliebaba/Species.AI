#include "pch.h"

#include "Check.h"
#include "Profile.h"
#include "RegionSimulation.h"
#include "StateSender.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>

namespace Species
{

namespace
{

std::uint16_t ClampToU16(double _value) noexcept
{
  return static_cast<std::uint16_t>(std::clamp(_value, 0.0, 65'535.0));
}

ReplicatedFigure FigureState(const Figure& _figure, std::uint8_t _team, StableId _selected)
{
  ReplicatedFigure state;
  state.team = _team;
  state.kind = _figure.kind;
  state.position = QuantizePosition(_figure.position);
  state.yaw = QuantizeYaw(_figure.front.x, _figure.front.z);
  state.health = ClampToU16(_figure.health);
  state.task =
    _figure.kind == FigureKind::Worker ? static_cast<std::uint8_t>(_figure.workerState) : static_cast<std::uint8_t>(_figure.leaderState);
  state.order = static_cast<std::uint8_t>(_figure.order);
  state.dead = _figure.dead;
  state.onGround = _figure.onGround;
  state.underControl = _figure.underControl;
  state.formation = _figure.formation;
  state.selected = _figure.id == _selected;
  return state;
}

ReplicatedEffect EffectState(const Effect& _effect)
{
  ReplicatedEffect state;
  state.team = _effect.team;
  state.kind = _effect.kind;
  state.position = QuantizePosition(_effect.position);
  state.yaw = QuantizeYaw(_effect.velocity.x, _effect.velocity.z);
  state.timer = _effect.kind == EffectKind::ShockwaveRing ? ClampToU16(_effect.activeTick) : ClampToU16(_effect.fuseTicks);
  state.size = _effect.kind == EffectKind::ShockwaveRing ? ClampToU16(std::round(_effect.sizeSeconds * EFFECT_SIZE_STEPS_PER_SECOND))
                                                         : std::uint16_t{0};
  return state;
}

bool IsTeamsOwn(const ReplicatedEffect& _effect) noexcept
{
  return _effect.kind == EffectKind::OrderMarker || _effect.kind == EffectKind::OrderTrailMark;
}

} // namespace

std::vector<ReplicatedObject> CollectReplicatedObjects(const RegionSimulation& _region)
{
  ENGINE_PROFILE_SCOPE("net.replication.collect");
  std::vector<ReplicatedObject> objects;
  for (std::size_t team = 0; team < _region.TeamCount(); ++team)
  {
    const RegionSimulation::Team& slot = _region.TeamAt(team);
    for (std::uint32_t n = 0; n < slot.figures.Capacity(); ++n)
    {
      if (slot.figures.Used(n))
      {
        const Figure& figure = slot.figures.At(n);
        objects.push_back({figure.id, FigureState(figure, static_cast<std::uint8_t>(team), slot.selectedId)});
      }
    }
  }
  for (std::uint32_t n = 0; n < _region.Effects().Capacity(); ++n)
  {
    if (_region.Effects().Used(n))
    {
      const Effect& effect = _region.Effects().At(n);
      objects.push_back({effect.id, EffectState(effect)});
    }
  }
  for (std::uint32_t n = 0; n < _region.Orbs().Capacity(); ++n)
  {
    if (_region.Orbs().Used(n))
    {
      const EssenceOrb& orb = _region.Orbs().At(n);
      objects.push_back({orb.id, ReplicatedOrb{orb.team, QuantizePosition(orb.position), orb.state, ClampToU16(orb.timerTicks)}});
    }
  }
  std::ranges::sort(objects, {}, &ReplicatedObject::id);
  return objects;
}

std::vector<ReplicatedObject> ViewForSteward(std::span<const ReplicatedObject> _objects, const RegionSimulation& _region, StableId _steward)
{
  const auto commands = [&_region, _steward](std::uint8_t _team) { return _region.CommanderOf(_team).sender == _steward; };
  std::vector<ReplicatedObject> view;
  view.reserve(_objects.size());
  for (const ReplicatedObject& object : _objects)
  {
    if (const auto* effect = std::get_if<ReplicatedEffect>(&object.state);
        effect != nullptr && IsTeamsOwn(*effect) && !commands(effect->team))
    {
      continue;
    }
    ReplicatedObject& seen = view.emplace_back(object);
    if (auto* figure = std::get_if<ReplicatedFigure>(&seen.state); figure != nullptr && !commands(figure->team))
    {
      figure->underControl = false;
      figure->selected = false;
    }
  }
  return view;
}

std::vector<StateSender::Outgoing> StateSender::Build(std::uint64_t _tick, std::span<const ReplicatedObject> _objects)
{
  ENGINE_PROFILE_SCOPE("net.replication.build");
  std::map<std::uint64_t, std::vector<StateRecord>> deltas; // by baseline tick
  std::vector<StateRecord> loose;                           // whole records and removals, which name no baseline
  std::vector<Known> known;                                 // what the client may hold after this tick, in id order
  known.reserve(std::max(m_known.size(), _objects.size()));
  auto object = _objects.begin();
  auto held = m_known.begin();
  while (object != _objects.end() || held != m_known.end())
  {
    if (held == m_known.end() || (object != _objects.end() && object->id < held->id))
    {
      // An object the client has never been sent: it enters whole.
      loose.push_back(StateRecord{object->id, RecordKind::Whole, object->state, 0, {}});
      known.push_back(Known{object->id, std::nullopt, 0});
      ++object;
      continue;
    }
    if (object == _objects.end() || held->id < object->id)
    {
      // An object that left: removed until the client acknowledges the removal.
      loose.push_back(RemovalRecord(held->id));
      known.push_back(*held);
      ++held;
      continue;
    }
    if (!held->acked || *held->acked != object->state)
    {
      const bool baseline = held->acked && _tick - held->ackedTick < STATE_BASELINE_TICKS_MAX;
      if (const std::optional<StateRecord> record = MakeRecord(object->id, object->state, baseline ? &*held->acked : nullptr))
      {
        if (record->kind == RecordKind::Delta)
        {
          deltas[held->ackedTick].push_back(*record);
        }
        else
        {
          loose.push_back(*record);
        }
      }
    }
    known.push_back(*held);
    ++object;
    ++held;
  }
  m_known = std::move(known);

  // The records that name no baseline ride with the largest group, so a tick takes as few datagrams as it can.
  if (!loose.empty())
  {
    if (deltas.empty())
    {
      deltas[_tick] = std::move(loose);
    }
    else
    {
      auto largest = std::ranges::max_element(deltas, {}, [](const auto& _group) { return _group.second.size(); });
      std::vector<StateRecord> merged;
      merged.reserve(largest->second.size() + loose.size());
      std::ranges::merge(largest->second, loose, std::back_inserter(merged), {}, &StateRecord::id, &StateRecord::id);
      largest->second = std::move(merged);
    }
  }

  std::vector<Outgoing> outgoing;
  for (const auto& [baselineTick, records] : deltas)
  {
    // A group of whole records and removals alone names no baseline, age 0.
    const auto age = static_cast<std::uint16_t>(_tick - baselineTick);
    for (StateUpdate& update : PackStateUpdates(_tick, age, records))
    {
      Outgoing& out = outgoing.emplace_back();
      out.held.reserve(update.records.size());
      for (const StateRecord& record : update.records)
      {
        std::optional<ReplicatedState> state;
        if (record.kind != RecordKind::Removed)
        {
          const auto found = std::ranges::lower_bound(_objects, record.id, {}, &ReplicatedObject::id);
          ENGINE_ASSERT(found != _objects.end() && found->id == record.id);
          state = found->state;
        }
        out.held.emplace_back(record.id, state);
      }
      out.update = std::move(update);
    }
  }
  // A tick with nothing to change still sends its tick, so the client knows the server is not late (ADR-084 item 4).
  if (outgoing.empty())
  {
    outgoing.push_back(Outgoing{StateUpdate{_tick, 0, {}}, {}});
  }
  return outgoing;
}

void StateSender::Sent(std::uint64_t _datagram, Outgoing&& _outgoing)
{
  m_inFlight.push_back(InFlight{_datagram, _outgoing.update.tick, std::move(_outgoing.held)});
}

void StateSender::Fates(std::span<const Neuron::DatagramOutcome> _outcomes, std::uint64_t _tick)
{
  for (const Neuron::DatagramOutcome& outcome : _outcomes)
  {
    const auto sent = std::ranges::find(m_inFlight, outcome.datagram, &InFlight::datagram);
    if (sent == m_inFlight.end())
    {
      continue; // an event datagram, or one sent before the world was replaced
    }
    if (outcome.fate == Neuron::DatagramFate::Acknowledged && sent->tick + STATE_BASELINE_TICKS_MAX > _tick)
    {
      for (const auto& [id, state] : sent->held)
      {
        const auto known = std::ranges::lower_bound(m_known, id, {}, &Known::id);
        if (known == m_known.end() || known->id != id)
        {
          continue; // its removal was acknowledged already
        }
        if (!state)
        {
          m_known.erase(known);
        }
        else if (!known->acked || known->ackedTick < sent->tick)
        {
          known->acked = state;
          known->ackedTick = sent->tick;
        }
      }
    }
    m_inFlight.erase(sent);
  }
  // A datagram with no fate after the baseline window can no longer serve as a baseline: it counts as lost.
  std::erase_if(m_inFlight, [_tick](const InFlight& _sent) { return _sent.tick + STATE_BASELINE_TICKS_MAX <= _tick; });
}

void StateSender::Forget()
{
  for (Known& known : m_known)
  {
    known.acked.reset();
  }
  m_inFlight.clear();
}

} // namespace Species
