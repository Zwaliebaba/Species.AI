#include "pch.h"

#include "Log.h"
#include "LogFloodScene.h"
#include "MarkerTimings.h"
#include "PlatformClock.h"
#include "PlatformThread.h"
#include "RunId.h"

#include <filesystem>
#include <format>
#include <latch>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>

namespace Performance
{

namespace
{

// A debug record of the length the simulation writes: three arguments and about 50 characters.
constexpr std::string_view FLOOD_FORMAT = "flood thread {} record {} at {:.1f} wu from its target";

} // namespace

LogFlood MeasureLogFlood(std::uint64_t _records)
{
  const std::uint64_t perThread = _records / LOG_FLOOD_THREADS;
  const std::uint64_t runId = Neuron::DrawRunId();
  const std::filesystem::path folder = std::filesystem::temp_directory_path() / std::format("SpeciesPerformance-{:016x}", runId);
  LogFlood flood;
  flood.records = perThread * LOG_FLOOD_THREADS;
  flood.threadNanoseconds.assign(LOG_FLOOD_THREADS, 0);
  {
    Neuron::PlatformClock clock(std::make_unique<Neuron::CounterTimeSource>());
    Neuron::Logger::Desc desc;
    desc.clock = &clock;
    desc.directory = folder;
    desc.runId = runId;
    desc.header = {{"product", "SpeciesPerformance"}, {"scene", "log-flood"}};
    Neuron::Logger logger(std::move(desc));
    std::latch start(LOG_FLOOD_THREADS);
    {
      std::vector<Neuron::PlatformThread> threads;
      threads.reserve(LOG_FLOOD_THREADS);
      for (std::uint32_t index = 0; index < LOG_FLOOD_THREADS; ++index)
      {
        threads.push_back(Neuron::PlatformThread::Start(std::format("perf.log_flood.{}", index),
                                                        [&start, &flood, index, perThread](const std::stop_token&)
                                                        {
                                                          start.arrive_and_wait();
                                                          const std::int64_t begin = Neuron::MarkerClockNanoseconds();
                                                          for (std::uint64_t record = 0; record < perThread; ++record)
                                                          {
                                                            const double distanceWu = static_cast<double>(record % 4096) * 0.25;
                                                            // A Release build folds a debug call away at compile time (DBG-064), so
                                                            // the flood takes the path of a record that passed the level test: the
                                                            // formatting into the thread's scratch buffer and the write into its ring.
                                                            Neuron::LogFormatted(Neuron::LogCategory::Diagnostics, Neuron::LogLevel::Debug,
                                                                                 FLOOD_FORMAT,
                                                                                 std::make_format_args(index, record, distanceWu));
                                                          }
                                                          flood.threadNanoseconds[index] = Neuron::MarkerClockNanoseconds() - begin;
                                                        }));
      }
      for (Neuron::PlatformThread& thread : threads)
      {
        thread.Join();
      }
    }
    logger.Flush();
    const Neuron::LogStats stats = logger.Stats();
    flood.written = stats.written;
    flood.dropped = stats.droppedDebug;
  }
  std::error_code error;
  std::filesystem::remove_all(folder, error);
  return flood;
}

} // namespace Performance
