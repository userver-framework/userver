#pragma once

#include <atomic>
#include <chrono>

#include <userver/cache/update_type.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

// TODO TAXICOMMON-2262 replace with `inline constexpr std::string_view`
inline constexpr const char* kStatisticsNameFull = "full";
inline constexpr const char* kStatisticsNameIncremental = "incremental";
inline constexpr const char* kStatisticsNameAny = "any";
inline constexpr const char* kStatisticsNameCurrentDocumentsCount =
    "current-documents-count";
inline constexpr const char* kStatisticsNameDump = "dump";

inline constexpr const char* kStatisticsNameHits = "hits";
inline constexpr const char* kStatisticsNameMisses = "misses";
inline constexpr const char* kStatisticsNameStale = "stale";
inline constexpr const char* kStatisticsNameBackground = "background-updates";
inline constexpr const char* kStatisticsNameHitRatio = "hit_ratio";

struct UpdateStatistics {
  std::atomic<size_t> update_attempt_count{0};
  std::atomic<size_t> update_no_changes_count{0};
  std::atomic<size_t> update_failures_count{0};

  std::atomic<size_t> documents_read_count{0};
  std::atomic<size_t> documents_parse_failures{0};

  std::atomic<std::chrono::steady_clock::time_point> last_update_start_time{{}};
  std::atomic<std::chrono::steady_clock::time_point>
      last_successful_update_start_time{{}};
  std::atomic<std::chrono::milliseconds> last_update_duration{{}};

  UpdateStatistics() = default;

  UpdateStatistics(const UpdateStatistics& other)
      : update_attempt_count(other.update_attempt_count.load()),
        update_no_changes_count(other.update_no_changes_count.load()),
        update_failures_count(other.update_failures_count.load()),
        documents_read_count(other.documents_read_count.load()),
        documents_parse_failures(other.documents_parse_failures.load()),
        last_update_start_time(other.last_update_start_time.load()),
        last_successful_update_start_time(
            other.last_successful_update_start_time.load()),
        last_update_duration(other.last_update_duration.load()) {}
};

UpdateStatistics CombineStatistics(const UpdateStatistics& a,
                                   const UpdateStatistics& b);

formats::json::Value StatisticsToJson(const UpdateStatistics& stats);

struct Statistics {
  UpdateStatistics full_update;
  UpdateStatistics incremental_update;
  std::atomic<size_t> documents_current_count{0};

  Statistics() = default;
};

class UpdateStatisticsScope final {
 public:
  UpdateStatisticsScope(Statistics& stats, cache::UpdateType type);

  ~UpdateStatisticsScope();

  void Finish(size_t documents_count);

  void FinishNoChanges();

  void IncreaseDocumentsReadCount(size_t add);

  void IncreaseDocumentsParseFailures(size_t add);

 private:
  Statistics& stats_;
  UpdateStatistics& update_stats_;
  bool finished_;
  const std::chrono::steady_clock::time_point update_start_time_;
};

}  // namespace cache

USERVER_NAMESPACE_END
