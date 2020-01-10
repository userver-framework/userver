#pragma once

#include <atomic>
#include <chrono>

#include <cache/update_type.hpp>
#include <formats/json/value.hpp>

namespace cache {

static const char* kStatisticsNameFull = "full";
static const char* kStatisticsNameIncremental = "incremental";
static const char* kStatisticsNameAny = "any";
static const char* kStatisticsNameCurrentDocumentsCount =
    "current-documents-count";

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

  // atomic<time_point> is invalid due to a missing noexcept in ctr,
  // so using atomic<duration instead> :-(
  std::atomic<std::chrono::system_clock::duration> last_update_start_time{{}};
  std::atomic<std::chrono::system_clock::duration>
      last_successful_update_start_time{};
  std::atomic<std::chrono::milliseconds> last_update_duration{};

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

  std::chrono::milliseconds GetAge() {
    auto diff = std::chrono::system_clock::now().time_since_epoch() -
                last_update_start_time.load();
    return std::chrono::duration_cast<std::chrono::milliseconds>(diff);
  }
};

UpdateStatistics CombineStatistics(const UpdateStatistics& a,
                                   const UpdateStatistics& b);

formats::json::Value StatisticsToJson(const UpdateStatistics& stats);

struct Statistics {
  UpdateStatistics full_update, incremental_update;
  std::atomic<size_t> documents_current_count{0};

  Statistics() = default;

  Statistics(const Statistics& other)
      : full_update(other.full_update),
        incremental_update(other.incremental_update),
        documents_current_count(other.documents_current_count.load()) {}
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
  const std::chrono::system_clock::duration update_start_time_;
};

}  // namespace cache
