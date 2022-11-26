#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <boost/regex.hpp>

#include <userver/dump/config.hpp>
#include <userver/dump/helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

const std::string kFilenameDateFormat = "%Y-%m-%dT%H%M%E6SZ";
const std::string kLegacyFilenameDateFormat = "%Y-%m-%dT%H:%M:%E6S";

struct DumpFileStats final {
  TimePoint update_time;
  std::string full_path;
  uint64_t format_version;
};

/// @brief Manages dump files on disk. Encapsulates file paths and naming scheme
/// and performs necessary bookkeeping.
/// @note The class is thread-safe, except for `Cleanup`
class DumpLocator final {
 public:
  explicit DumpLocator(Config static_config);

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

  /// @brief Modifies the update time for a dump
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @return `true` on success, `false` if the dump is not available
  bool BumpDumpTime(TimePoint old_update_time, TimePoint new_update_time);

  /// @brief Removes old dumps and tmp files
  /// @note The operation is blocking, and should run in FS TaskProcessor
  /// @warning Must not be called concurrently with `RegisterNewDump`
  void Cleanup();

 private:
  enum class FileFormatType { kNormal, kTmp };

  std::optional<DumpFileStats> ParseDumpName(std::string full_path) const;

  std::optional<DumpFileStats> GetLatestDumpImpl() const;

  std::string GenerateDumpPath(TimePoint update_time) const;

  TimePoint MinAcceptableUpdateTime() const;

  static std::string GenerateFilenameRegex(FileFormatType type);

  static TimePoint Round(std::chrono::system_clock::time_point);

  const Config config_;
  const boost::regex filename_regex_;
  const boost::regex tmp_filename_regex_;
};

}  // namespace dump

USERVER_NAMESPACE_END
