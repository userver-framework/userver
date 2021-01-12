#pragma once

#include <atomic>
#include <chrono>

#include <cache/update_type.hpp>
#include <formats/json/value.hpp>

namespace cache {

// TODO TAXICOMMON-2262 replace with `inline constexpr std::string_view`
static const char* kStatisticsNameFull = "full";
static const char* kStatisticsNameIncremental = "incremental";
static const char* kStatisticsNameAny = "any";
static const char* kStatisticsNameCurrentDocumentsCount =
    "current-documents-count";
static const char* kStatisticsNameDump = "dump";

static const char* kStatisticsNameHits = "hits";
static const char* kStatisticsNameMisses = "misses";
static const char* kStatisticsNameStale = "stale";
static const char* kStatisticsNameBackground = "background-updates";
static const char* kStatisticsNameHitRatio = "hit_ratio";

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

struct DumpStatistics {
  std::atomic<bool> is_loaded_{false};
  std::atomic<std::chrono::milliseconds> load_duration_{{}};

  std::atomic<std::chrono::steady_clock::time_point>
      last_nontrivial_write_start_time{{}};
  std::atomic<std::chrono::milliseconds> last_nontrivial_write_duration{{}};
};

formats::json::Value StatisticsToJson(const DumpStatistics& stats);

struct Statistics {
  UpdateStatistics full_update;
  UpdateStatistics incremental_update;
  std::atomic<size_t> documents_current_count{0};
  DumpStatistics dump;

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
