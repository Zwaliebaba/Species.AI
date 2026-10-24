#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "Check.h"
#include "CommandJournal.h"
#include "Files.h"
#include "Growth.h"
#include "PlatformThread.h"
#include "RegionSimulation.h"
#include "SettlementTasks.h"
#include "SimulationRecord.h"
#include "TerrainField.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <array>
#include <atomic>
#include <cmath>
#include <exception>
#include <fstream>
#include <mutex>
#include <stop_token>
#include <system_error>
#include <utility>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

std::mutex g_checksMutex;
std::vector<std::string> g_checks;

#if ENGINE_DEVELOPMENT
void OnDevelopmentCheck(std::string_view _message)
{
  const std::lock_guard lock(g_checksMutex);
  g_checks.emplace_back(_message);
}
#endif

} // namespace

namespace
{

std::atomic<std::uint64_t> g_folderCount{0};

} // namespace

TemporaryFolder::TemporaryFolder()
{
  std::error_code error;
  m_path = std::filesystem::temp_directory_path(error) / L"GameLogicTests" /
           std::format(L"{}-{}", GetCurrentProcessId(), g_folderCount.fetch_add(1, std::memory_order_relaxed));
  (void)std::filesystem::remove_all(m_path, error);
  (void)std::filesystem::create_directories(m_path, error);
}

TemporaryFolder::~TemporaryFolder()
{
  try
  {
    std::error_code error;
    (void)std::filesystem::remove_all(m_path, error);
  }
  catch (...)
  {
    // A folder left behind is harmless: the next run of this process id removes it first.
    OutputDebugStringW(L"GameLogicTests: a temporary folder was not removed\n");
  }
}

std::filesystem::path RepositoryContentRoot()
{
  return std::filesystem::path(__FILE__).parent_path().parent_path() / L"GameData";
}

const Species::TuningMvp& CommittedTuning()
{
  static const Species::TuningMvp COMMITTED_TUNING = []
  {
    const Neuron::RecordResult<Species::TuningMvp> tuning = Species::ReadTuningMvpSource(RepositoryContentRoot());
    Assert::IsTrue(tuning.has_value(), L"the committed tuning record reads whole");
    return tuning.value_or(Species::TuningMvp{});
  }();
  return COMMITTED_TUNING;
}

std::shared_ptr<const Species::RegionTerrain> FlatTerrain(double _height)
{
  const Species::TuningMvp& tuning = CommittedTuning();
  Neuron::TerrainDefinition definition;
  definition.cellSize = tuning.wld.terrain.cellSize;
  definition.originX = static_cast<double>(tuning.mvp.world.regionCell) * tuning.wld.region.side;
  definition.originZ = definition.originX;
  definition.worldSizeX = static_cast<std::int32_t>(tuning.wld.region.side);
  definition.worldSizeZ = definition.worldSizeX;
  definition.outsideHeight = _height;
  auto terrain = std::make_shared<Species::RegionTerrain>();
  terrain->field = Neuron::TerrainField::Generate(definition, {}, {});
  return terrain;
}

void RunConcurrently(const std::vector<std::function<void()>>& _jobs)
{
  std::vector<std::exception_ptr> failures(_jobs.size());
  std::vector<Neuron::PlatformThread> threads;
  threads.reserve(_jobs.size());
  for (std::size_t index = 0; index < _jobs.size(); ++index)
  {
    threads.push_back(Neuron::PlatformThread::Start("test.concurrent",
                                                    [&_jobs, &failures, index](const std::stop_token&)
                                                    {
                                                      try
                                                      {
                                                        _jobs[index]();
                                                      }
                                                      catch (...)
                                                      {
                                                        failures[index] = std::current_exception();
                                                      }
                                                    }));
  }
  for (Neuron::PlatformThread& thread : threads)
  {
    thread.Join();
  }
  for (const std::exception_ptr& failure : failures)
  {
    if (failure)
    {
      std::rethrow_exception(failure);
    }
  }
}

std::shared_ptr<const Species::RegionTerrain> ValleyTerrain()
{
  static const std::shared_ptr<const Species::RegionTerrain> VALLEY =
    std::make_shared<const Species::RegionTerrain>(*Species::GenerateRegionTerrain(1, CommittedTuning()));
  return VALLEY;
}

std::string ReadText(const std::filesystem::path& _path)
{
  const auto bytes = Neuron::ReadWholeFile(_path);
  if (!bytes)
  {
    return {};
  }
  return std::string(reinterpret_cast<const char*>(bytes->data()), bytes->size());
}

void WriteText(const std::filesystem::path& _path, std::string_view _text)
{
  std::error_code error;
  (void)std::filesystem::create_directories(_path.parent_path(), error);
  std::ofstream file(_path, std::ios::binary | std::ios::trunc);
  file.write(_text.data(), static_cast<std::streamsize>(_text.size()));
}

std::vector<std::string> TakeDevelopmentChecks()
{
  const std::lock_guard lock(g_checksMutex);
  std::vector<std::string> checks;
  checks.swap(g_checks);
  return checks;
}

// Development checks become test failures (DBG-072): the hook records them, and every test class checks the record.
TEST_MODULE_INITIALIZE(ModuleInitialize)
{
#if ENGINE_DEVELOPMENT
  Neuron::SetDevelopmentCheckHook(&OnDevelopmentCheck);
#endif
}

TEST_MODULE_CLEANUP(ModuleCleanup)
{
#if ENGINE_DEVELOPMENT
  Neuron::SetDevelopmentCheckHook(nullptr);
#endif
}

namespace
{

// A record of one root node at the identity, its ports on a ring, its entrance and door where it has them.
Neuron::SimulationRecord TestRecord(std::uint32_t _ports, bool _entrance, bool _door)
{
  constexpr double RING_WU = 20.0;
  constexpr double ENTRANCE_WU = 25.0;
  constexpr std::array<float, 16> IDENTITY{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
  Neuron::SimulationRecord record;
  record.nodes.push_back(Neuron::SimulationNode{"body", std::nullopt, IDENTITY});
  const auto at = [&](Neuron::SocketKind _kind, std::uint32_t _index, double _x, double _z)
  {
    Neuron::SimulationSocket socket{_kind, _index, 0, IDENTITY};
    socket.transform[12] = static_cast<float>(_x);
    socket.transform[14] = static_cast<float>(_z);
    record.sockets.push_back(socket);
  };
  for (std::uint32_t port = 0; port < _ports; ++port)
  {
    const double angle = 6.283185307179586 * static_cast<double>(port) / static_cast<double>(_ports);
    at(Neuron::SocketKind::Port, port, RING_WU * std::cos(angle), RING_WU * std::sin(angle));
  }
  if (_door)
  {
    at(Neuron::SocketKind::Door, 0, 0.0, ENTRANCE_WU);
  }
  if (_entrance)
  {
    at(Neuron::SocketKind::Entrance, 0, 0.0, -ENTRANCE_WU);
  }
  record.bounds = Neuron::BoundingSphere{{0.0F, 5.0F, 0.0F}, 15.0F};
  return record;
}

} // namespace

Species::StructureRecords TestStructureRecords()
{
  Species::StructureRecords records;
  records.wellspring = TestRecord(5, true, true);
  records.dwelling = TestRecord(6, false, false);
  records.storehouse = TestRecord(4, true, false);
  records.resourceSite = TestRecord(4, false, false);
  records.resourceSiteCentral = TestRecord(12, false, false);
  records.turretMount = TestRecord(4, false, false);
  records.turretHead = TestRecord(0, false, false);
  records.turretBarrel = TestRecord(0, false, false);
  // The turret's barrels at the head's barrel mount, 10 wu up and 7.5 wu forward, with a muzzle for each port, 14.5 wu
  // further forward (the drafted recipe's, ADR-128).
  constexpr std::array<float, 16> IDENTITY{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
  Neuron::SimulationSocket mount{Neuron::SocketKind::BarrelMount, 0, 0, IDENTITY};
  mount.transform[13] = 10.0F;
  mount.transform[14] = 7.5F;
  records.turretHead.sockets.push_back(mount);
  for (std::uint32_t barrel = 0; barrel < 4; ++barrel)
  {
    Neuron::SimulationSocket muzzle{Neuron::SocketKind::Muzzle, barrel, 0, IDENTITY};
    muzzle.transform[12] = barrel % 2 == 0 ? -2.0F : 2.0F;
    muzzle.transform[13] = barrel < 2 ? 1.4F : -1.4F;
    muzzle.transform[14] = 15.5F;
    records.turretBarrel.sockets.push_back(muzzle);
  }
  return records;
}

const Species::StructureRecords& TestRecords()
{
  static const Species::StructureRecords RECORDS = TestStructureRecords();
  return RECORDS;
}

std::unique_ptr<Species::WorldSimulation> SettledWorld(std::uint8_t _site)
{
  Species::WorldStart start;
  start.key = SETTLED_TEST_KEY;
  start.seed = ValleyTerrain()->seed;
  start.stewardSite = _site;
  return Species::StartWorld(start, ValleyTerrain(), CommittedTuning(), &TestRecords());
}

std::vector<std::byte> RegionStateOf(const Species::RegionSimulation& _region, std::uint32_t _version)
{
  // The counts' bounds of the sections each version appended: version 11's grievances, version 12's claims and moved
  // areas, version 13's expeditions, battles, battle results and respites. Its statistics must be empty below version 15,
  // whose kinds were written narrower.
  constexpr std::array<std::pair<std::uint32_t, std::uint64_t>, 7> COUNTS{
    {{11, 65'535}, {12, 36}, {12, 9}, {13, 4}, {13, 4}, {13, 65'535}, {13, 4}}};
  Neuron::BitWriter current;
  _region.WriteState(current);
  std::size_t bits = current.BitCount();
  for (const auto& [version, bound] : COUNTS)
  {
    bits -= version > _version ? Neuron::BitsFor(bound) : 0;
  }
  // Version 18's home ports, the last section, for each figure, version 17's growth holds before it, a bit for each
  // settlement, and version 16's claim ends, version 15's renown and version 14's AI settlements' decisions before them,
  // go whole.
  if (_version < Species::STATE_VERSION_WITH_HOME_PORTS)
  {
    Neuron::BitWriter homes;
    for (std::size_t team = 0; team < _region.TeamCount(); ++team)
    {
      const auto& figures = _region.TeamAt(team).figures;
      for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
      {
        if (figures.Used(slot))
        {
          Species::WriteHomePort(homes, figures.At(slot));
        }
      }
    }
    bits -= homes.BitCount();
  }
  if (_version < Species::STATE_VERSION_WITH_GROWTH_HOLD)
  {
    bits -= _region.Settlements().size();
  }
  if (_version < Species::STATE_VERSION_WITH_CLAIM_COOLDOWN)
  {
    Neuron::BitWriter ends;
    ends.WriteBounded(_region.ClaimEnds().size(), Species::CLAIM_AREA_MAX + 1);
    for (const Species::ClaimEnd& end : _region.ClaimEnds())
    {
      Species::WriteClaimEnd(ends, end);
    }
    bits -= ends.BitCount();
  }
  if (_version < Species::STATE_VERSION_WITH_RENOWN)
  {
    Neuron::BitWriter renown;
    renown.WriteBounded(_region.Renown().size(), 4);
    for (const Species::SettlementRenown& settlement : _region.Renown())
    {
      Species::WriteSettlementRenown(renown, settlement);
    }
    bits -= renown.BitCount();
  }
  if (_version < Species::STATE_VERSION_WITH_SETTLEMENT_AI)
  {
    Neuron::BitWriter decisions;
    decisions.WriteBounded(_region.SettlementAis().size(), 4);
    for (const Species::SettlementAi& ai : _region.SettlementAis())
    {
      Species::WriteSettlementAi(decisions, ai);
    }
    bits -= decisions.BitCount();
  }
  Neuron::BitReader whole(current.Bytes());
  Neuron::BitWriter older;
  for (std::size_t done = 0; done < bits;)
  {
    const auto chunk = static_cast<std::uint32_t>(std::min<std::size_t>(64, bits - done));
    older.WriteBits(whole.ReadBits(chunk), chunk);
    done += chunk;
  }
  return older.Take();
}

std::unique_ptr<Species::WorldSimulation> ValleyWorld(const Neuron::SipHashKey& _key, const Species::TuningMvp& _tuning)
{
  Species::WorldStart start;
  start.key = _key;
  start.seed = ValleyTerrain()->seed;
  start.stewardSite = std::uint8_t{2};
  start.aiSettlements = 3;
  return Species::StartWorld(start, ValleyTerrain(), _tuning, &TestRecords());
}

std::vector<Neuron::CookedEntry> StructureRecordEntries()
{
  Species::StructureRecords records = TestStructureRecords();
  std::vector<Neuron::CookedEntry> entries;
  for (std::size_t place = 0; place < Species::STRUCTURE_RECORD_KEYS.size(); ++place)
  {
    Neuron::CookedEntry entry;
    entry.entry.key = std::string(Species::STRUCTURE_RECORD_KEYS[place]);
    entry.entry.type = Neuron::ContentType::SimRecord;
    entry.entry.simulationRelevant = true;
    entry.bytes = Neuron::WriteSimulationRecord(Species::RecordAt(records, place));
    entries.push_back(std::move(entry));
  }
  return entries;
}

} // namespace GameLogicTests
