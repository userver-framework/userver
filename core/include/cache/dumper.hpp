#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <boost/regex.hpp>

#include <components/loggable_component_base.hpp>
#include <engine/async.hpp>
#include <rcu/rcu.hpp>

#include <cache/cache_config.hpp>

namespace cache {

inline const std::string kDumpFilenameDateFormat = "%Y-%m-%dT%H:%M:%E6S";

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;

struct DumpContents {
  std::string contents;
  TimePoint update_time;
};

/// @brief Manages cache dumps on disk
/// @note The class is thread-safe
class Dumper final {
 public:
  Dumper(CacheConfigStatic&& config, engine::TaskProcessor& fs_task_processor,
         std::string_view cache_name);

  /// @brief Creates a new cache dump file
  /// @return `true` on success
  bool WriteNewDump(DumpContents dump);

  /// Reads the latest dump file, if available and fresh enough
  std::optional<DumpContents> ReadLatestDump();

  /// @brief Modifies the update time for a cache dump
  /// @return `true` on success, `false` if the dump is not available
  bool BumpDumpTime(TimePoint old_update_time, TimePoint new_update_time);

  /// Removes old dumps and tmp files
  /// @warning Must not be called concurrently with `WriteNewDump`
  void Cleanup();

  /// Changes the config used for new operations
  void SetConfig(const CacheConfigStatic& config);

 private:
  struct ParsedDumpName {
    std::string filename;
    TimePoint update_time;
    uint64_t format_version;
  };

  enum class FileFormatType { kNormal, kTmp };

  std::optional<ParsedDumpName> ParseDumpName(std::string filename) const;

  std::optional<std::string> GetLatestDumpNameBlocking(
      const CacheConfigStatic& config) const;

  std::optional<std::string> GetLatestDumpName(
      const CacheConfigStatic& config) const;

  void CleanupBlocking(const CacheConfigStatic& config);

  static std::string FilenameToPath(std::string_view filename,
                                    const CacheConfigStatic& config);

  static std::string GenerateDumpPath(TimePoint update_time,
                                      const CacheConfigStatic& config);

  static std::string GenerateFilenameRegex(FileFormatType type);

  static TimePoint MinAcceptableUpdateTime(const CacheConfigStatic& config);

  static TimePoint Round(std::chrono::system_clock::time_point);

 private:
  rcu::Variable<CacheConfigStatic> config_;
  engine::TaskProcessor& fs_task_processor_;
  const std::string_view cache_name_;
  const boost::regex filename_regex_;
  const boost::regex tmp_filename_regex_;
};

}  // namespace cache
