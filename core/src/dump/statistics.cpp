#include <dump/statistics.hpp>

#include <formats/json/value_builder.hpp>

namespace dump {

formats::json::Value Serialize(const Statistics& stats,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder result(formats::json::Type::kObject);

  const bool is_loaded = stats.is_loaded.load();
  result["is-loaded-from-dump"] = is_loaded ? 1 : 0;
  if (is_loaded) {
    result["load-duration-ms"] = stats.load_duration.load().count();
  }
  result["is-current-from-dump"] = stats.is_current_from_dump.load() ? 1 : 0;

  const bool dump_written = stats.last_nontrivial_write_start_time.load() !=
                            std::chrono::steady_clock::time_point{};
  if (dump_written) {
    formats::json::ValueBuilder write(formats::json::Type::kObject);
    write["time-from-start-ms"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() -
            stats.last_nontrivial_write_start_time.load())
            .count();
    write["duration-ms"] = stats.last_nontrivial_write_duration.load().count();
    write["size-kb"] = stats.last_written_size.load() / 1024;
    result["last-nontrivial-write"] = write.ExtractValue();
  }

  return result.ExtractValue();
}

}  // namespace dump
