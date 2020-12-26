#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <boost/regex.hpp>

#include <rcu/rcu.hpp>

#include <cache/cache_config.hpp>
#include <cache/dump/operations_file.hpp>

namespace cache::dump {

inline const std::string kFilenameDateFormat = "%Y-%m-%dT%H:%M:%E6S";

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;

/// @brief Manages cache dumps on disk
/// @note The class is thread-safe
class DumpManager final {
 public:
  DumpManager(CacheConfigStatic&& config, std::string_view cache_name);

  /// @brief Creates a new empty cache dump file
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @returns The file handle, or `nullopt` on a filesystem error
  std::optional<FileWriter> StartWriter(TimePoint update_time);

  struct StartReaderResult {
    FileReader contents;
    TimePoint update_time;
  };

  /// @brief Opens the latest cache dump file
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @returns The file handle if available and fresh enough,
  /// or `nullopt` otherwise
  std::optional<StartReaderResult> StartReader();

  /// @brief Modifies the update time for a cache dump
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @return `true` on success, `false` if the dump is not available
  bool BumpDumpTime(TimePoint old_update_time, TimePoint new_update_time);

  /// @brief Removes old dumps and tmp files
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @warning Must not be called concurrently with `StartWriter`
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

  std::optional<std::string> GetLatestDumpName(
      const CacheConfigStatic& config) const;

  void DoCleanup(const CacheConfigStatic& config);

  static std::string FilenameToPath(std::string_view filename,
                                    const CacheConfigStatic& config);

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
