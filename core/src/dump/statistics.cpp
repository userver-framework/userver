#include <dump/statistics.hpp>

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

void DumpMetric(utils::statistics::Writer& writer, const Statistics& stats) {
  const bool is_loaded = stats.is_loaded;
  writer["is-loaded-from-dump"] = is_loaded ? 1 : 0;
  if (is_loaded) {
    writer["load-duration-ms"] = stats.load_duration.load().count();
  }
  writer["is-current-from-dump"] = stats.is_current_from_dump.load() ? 1 : 0;

  const bool dump_written = stats.last_nontrivial_write_start_time.load() !=
                            std::chrono::steady_clock::time_point{};
  if (dump_written) {
    auto write = writer["last-nontrivial-write"];
    write["time-from-start-ms"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() -
            stats.last_nontrivial_write_start_time.load())
            .count();
    write["duration-ms"] = stats.last_nontrivial_write_duration.load().count();
    write["size-kb"] = stats.last_written_size.load() / 1024;
  }
}

}  // namespace dump

USERVER_NAMESPACE_END
