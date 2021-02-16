#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <boost/regex.hpp>

#include <rcu/rcu.hpp>

#include <cache/cache_config.hpp>

namespace cache::dump {

inline const std::string kFilenameDateFormat = "%Y-%m-%dT%H:%M:%E6S";

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;

struct DumpFileStats final {
  TimePoint update_time;
  std::string full_path;
  uint64_t format_version;
};

/// @brief Manages cache dump files on disk. Encapsulates file paths and naming
/// scheme and performs necessary bookkeeping.
/// @note The class is thread-safe, except for `Cleanup`
class DumpManager final {
 public:
  DumpManager(CacheConfigStatic&& config, std::string_view cache_name);

  /// @brief Prepare the place for a new dump
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @note The actual creation of the file is a caller's responsibility
  /// @throws On a filesystem error
  DumpFileStats RegisterNewDump(TimePoint update_time);

  /// @brief Finds the latest suitable dump
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @returns The full path of the dump if available and fresh enough,
  /// or `nullopt` otherwise
  std::optional<DumpFileStats> GetLatestDump() const;

  /// @brief Modifies the update time for a cache dump
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @return `true` on success, `false` if the dump is not available
  bool BumpDumpTime(TimePoint old_update_time, TimePoint new_update_time);

  /// @brief Removes old dumps and tmp files
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @warning Must not be called concurrently with `RegisterNewDump`
  void Cleanup();

  /// Changes the config used for new operations
  void SetConfig(const CacheConfigStatic& config);

 private:
  enum class FileFormatType { kNormal, kTmp };

  std::optional<DumpFileStats> ParseDumpName(std::string full_path) const;

  std::optional<DumpFileStats> GetLatestDump(
      const CacheConfigStatic& config) const;

  void DoCleanup(const CacheConfigStatic& config);

  static std::string GenerateDumpPath(TimePoint update_time,
                                      const CacheConfigStatic& config);

  static std::string GenerateFilenameRegex(FileFormatType type);

  static TimePoint MinAcceptableUpdateTime(const CacheConfigStatic& config);

  static TimePoint Round(std::chrono::system_clock::time_point);

 private:
  rcu::Variable<CacheConfigStatic> config_;
  const std::string_view cache_name_;
  const boost::regex filename_regex_;
  const boost::regex tmp_filename_regex_;
};

}  // namespace cache::dump
