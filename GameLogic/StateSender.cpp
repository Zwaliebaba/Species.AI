#include "pch.h"

#include "Check.h"
#include "Construction.h"
#include "PresentationEvents.h"
#include "Profile.h"
#include "RegionSimulation.h"
#include "SeenLayer.h"
#include "StateSender.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <numbers>

namespace Species
{

namespace
{

std::uint16_t ClampToU16(double _value) noexcept
{
  return static_cast<std::uint16_t>(std::clamp(_value, 0.0, 65'535.0));
}

ReplicatedFigure FigureState(const Figure& _figure, std::uint8_t _team, StableId _selected, const RouteStore& _routes)
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
  state.loaded = _figure.carriedMilliRu > 0; // ADR-139
  // O78, ADR-125: the flag pointer aims at the first point after the start of a Waypoint order's route (R-UNIT-191).
  if (_figure.kind == FigureKind::Leader && _figure.order == LeaderOrder::Waypoint)
  {
    const std::vector<Neuron::Vec3>* points = _routes.Find(_figure.route);
    if (points != nullptr && points->size() > 1)
    {
      state.target = QuantizePosition((*points)[1]);
    }
  }
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

// WLD-242's public layer (ADR-158): the sites and the settlements' cores, which every client sees wherever they are.
bool IsPublic(StructureKind _kind) noexcept
{
  return _kind == StructureKind::Wellspring || _kind == StructureKind::ResourceSite || _kind == StructureKind::ResourceSiteCentral;
}

// ADR-139: a structure as a client sees it.
ReplicatedStructure StructureState(const Structure& _structure, const TuningMvp& _tuning)
{
  ReplicatedStructure state;
  state.owner = _structure.owner;
  state.kind = _structure.kind;
  state.position = QuantizePosition(_structure.position);
  state.yaw = QuantizeYaw(_structure.front.x, _structure.front.z);
  state.built = _structure.built;
  state.active = _structure.active;
  if (!_structure.built)
  {
    // The status's progress (ADR-136), in thousandths of the work the site needs.
    const std::int64_t required = std::max<std::int64_t>(1, RequiredWorkTicks(_structure, _tuning));
    state.progress = static_cast<std::uint16_t>(
      std::clamp<std::int64_t>(_structure.workTicks * PROGRESS_PERMILLE_MAX / required, 0, PROGRESS_PERMILLE_MAX));
  }
  for (std::size_t port = 0; port < std::min(_structure.ports.size(), STRUCTURE_PORTS_MAX); ++port)
  {
    if (_structure.ports[port].occupant)
    {
      state.mannedPorts = static_cast<std::uint16_t>(state.mannedPorts | (1u << port));
    }
  }
  if (_structure.turret)
  {
    const TurretState& turret = *_structure.turret;
    state.turretTeam = turret.team;
    state.turretHealth = ClampToU16(turret.health);
    state.aimYaw = QuantizeYaw(turret.headFront.x, turret.headFront.z);
    // The barrels' up vector leans from vertical towards the head's front as they aim (R-BLDG-416).
    const double lean = turret.barrelUp.x * turret.headFront.x + turret.barrelUp.z * turret.headFront.z;
    const double turns = std::atan2(lean, turret.barrelUp.y) / (2.0 * std::numbers::pi);
    constexpr double PITCH_LIMIT = static_cast<double>(std::int64_t{1} << (PITCH_BITS - 1));
    state.aimPitch =
      static_cast<std::int16_t>(std::clamp(std::isfinite(turns) ? std::round(turns * YAW_STEPS) : 0.0, -PITCH_LIMIT, PITCH_LIMIT - 1.0));
  }
  return state;
}

// An amount of the stores in their replicated field.
std::uint64_t StoredMilliRu(std::int64_t _milliRu) noexcept
{
  constexpr std::uint64_t STORES_LIMIT = (std::uint64_t{1} << STORES_BITS) - 1;
  return std::min(static_cast<std::uint64_t>(std::max<std::int64_t>(0, _milliRu)), STORES_LIMIT);
}

// ADR-139: a settlement as its commander's client sees it; ViewForSteward clears what other clients do not see.
ReplicatedSettlement SettlementState(const RegionSimulation& _region, const Settlement& _settlement)
{
  constexpr std::uint32_t RADIUS_LIMIT = (std::uint32_t{1} << SETTLEMENT_RADIUS_BITS) - 1;
  const TuningMvp& tuning = _region.Tuning();
  ReplicatedSettlement state;
  state.site = _settlement.site;
  state.team = _settlement.team;
  state.color = _settlement.cladan.color;
  state.storesMilliRu = StoredMilliRu(_settlement.storesMilliRu);
  state.capacityMilliRu = StoredMilliRu(StoreCapacityMilliRu(tuning, ActiveCount(_region, _settlement.team, StructureKind::Storehouse)));
  state.population = static_cast<std::uint16_t>(std::min<std::uint32_t>(Population(_region, _settlement), 65'535));
  state.limit = static_cast<std::uint16_t>(std::min<std::uint32_t>(PopulationLimitOf(_region, _settlement), 65'535));
  state.radius = static_cast<std::uint32_t>(
    std::clamp(std::round(_settlement.territoryRadius * POSITION_STEPS_PER_UNIT), 0.0, static_cast<double>(RADIUS_LIMIT)));
  return state;
}

// ADR-139: the chunks of a raster that hold a cell, as objects of the site's layer.
void CollectCells(std::vector<ReplicatedObject>& _objects, const CellRaster* _raster, CellLayer _layer, std::uint8_t _site)
{
  const std::optional<CellRaster::ChunkBounds> bounds = _raster != nullptr ? _raster->HeldChunks() : std::nullopt;
  if (!bounds)
  {
    return;
  }
  // A chunk's place travels in 8 bits on each axis, which every region's rasters fit (ADR-139).
  for (std::int32_t z = bounds->firstZ; z <= std::min(bounds->lastZ, 255); ++z)
  {
    for (std::int32_t x = bounds->firstX; x <= std::min(bounds->lastX, 255); ++x)
    {
      if (const std::uint64_t cells = _raster->ChunkCells(x, z); cells != 0)
      {
        const auto chunkX = static_cast<std::uint8_t>(x);
        const auto chunkZ = static_cast<std::uint8_t>(z);
        _objects.push_back({CellChunkObjectId(_layer, _site, chunkX, chunkZ), ReplicatedCells{_layer, _site, chunkX, chunkZ, cells}});
      }
    }
  }
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
        objects.push_back({figure.id, FigureState(figure, static_cast<std::uint8_t>(team), slot.selectedId, _region.Routes())});
      }
    }
  }
  for (std::uint32_t n = 0; n < _region.Effects().Capacity(); ++n)
  {
    // A turret round is drawn from its events, as a laser bolt is, and does not travel (ADR-133).
    if (_region.Effects().Used(n) && _region.Effects().At(n).kind != EffectKind::TurretRound)
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
  // M2 task T11 (ADR-139): the structures, the settlements and their territories and seen layers.
  for (std::uint32_t n = 0; n < _region.Structures().Capacity(); ++n)
  {
    if (_region.Structures().Used(n))
    {
      const Structure& structure = _region.Structures().At(n);
      objects.push_back({structure.id, StructureState(structure, _region.Tuning())});
    }
  }
  for (const Settlement& settlement : _region.Settlements())
  {
    objects.push_back({SettlementObjectId(settlement.site), SettlementState(_region, settlement)});
    CollectCells(objects, _region.TerritoryOf(settlement.site), CellLayer::Territory, settlement.site);
    CollectCells(objects, _region.SeenLayerOf(settlement.site), CellLayer::Seen, settlement.site);
  }
  std::ranges::sort(objects, {}, &ReplicatedObject::id);
  return objects;
}

std::vector<ReplicatedObject> ViewForSteward(std::span<const ReplicatedObject> _objects, const RegionSimulation& _region, StableId _steward)
{
  const auto commands = [&_region, _steward](std::uint8_t _team) { return _region.CommanderOf(_team).sender == _steward; };
  // MVP-097 (ADR-158): another team's figures, effects and orbs, and another's structures, only where the seen layers of
  // the steward's settlements hold them; the settlements, their territories and Wellsprings and the resource sites are
  // the public layer (MVP-016, WLD-242).
  const Sight sight = Sight::OfSteward(_region, _steward);
  const auto sees = [&sight](const QuantizedPosition& _position)
  {
    const Neuron::Vec3 position = PositionOf(_position);
    return sight.Sees(position.x, position.z);
  };
  std::vector<ReplicatedObject> view;
  view.reserve(_objects.size());
  for (const ReplicatedObject& object : _objects)
  {
    if (const auto* effect = std::get_if<ReplicatedEffect>(&object.state);
        effect != nullptr && !commands(effect->team) && (IsTeamsOwn(*effect) || !sees(effect->position)))
    {
      continue;
    }
    if (const auto* figure = std::get_if<ReplicatedFigure>(&object.state);
        figure != nullptr && !commands(figure->team) && !sees(figure->position))
    {
      continue;
    }
    if (const auto* orb = std::get_if<ReplicatedOrb>(&object.state); orb != nullptr && !commands(orb->team) && !sees(orb->position))
    {
      continue;
    }
    const auto* structure = std::get_if<ReplicatedStructure>(&object.state);
    const bool hidden = structure != nullptr && !(structure->owner && commands(*structure->owner)) && !sees(structure->position);
    if (hidden && !IsPublic(structure->kind))
    {
      continue;
    }
    // MVP-097 (ADR-139): a settlement's seen layer goes to its commander only.
    if (const auto* cells = std::get_if<ReplicatedCells>(&object.state); cells != nullptr && cells->layer == CellLayer::Seen)
    {
      const Settlement* settlement = _region.SettlementAt(cells->site);
      if (settlement == nullptr || !commands(settlement->team))
      {
        continue;
      }
    }
    ReplicatedObject& seen = view.emplace_back(object);
    if (hidden)
    {
      // A site outside the seen layers shows its place and kind, not the Workers at its ports (WLD-242).
      std::get<ReplicatedStructure>(seen.state).mannedPorts = 0;
    }
    if (auto* figure = std::get_if<ReplicatedFigure>(&seen.state); figure != nullptr && !commands(figure->team))
    {
      figure->underControl = false;
      figure->selected = false;
    }
    // A settlement's stores and population are its commander's (ADR-139).
    if (auto* settlement = std::get_if<ReplicatedSettlement>(&seen.state); settlement != nullptr && !commands(settlement->team))
    {
      settlement->storesMilliRu = 0;
      settlement->capacityMilliRu = 0;
      settlement->population = 0;
      settlement->limit = 0;
    }
  }
  return view;
}

std::vector<EffectEvent> EventsForSteward(std::span<const EffectEvent> _events, const RegionSimulation& _region, StableId _steward)
{
  const Sight sight = Sight::OfSteward(_region, _steward);
  std::vector<EffectEvent> events;
  for (const EffectEvent& event : _events)
  {
    const bool commanded = _region.CommanderOf(event.team).sender == _steward;
    if (commanded || (!IsCommandersEvent(event.kind) && sight.Sees(event.position.x, event.position.z)))
    {
      events.push_back(event);
    }
  }
  return events;
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
