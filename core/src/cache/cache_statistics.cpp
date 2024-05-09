#include <userver/cache/cache_statistics.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace {

constexpr const char* kStatisticsNameFull = "full";
constexpr const char* kStatisticsNameIncremental = "incremental";
constexpr const char* kStatisticsNameAny = "any";
constexpr const char* kStatisticsNameCurrentDocumentsCount =
    "current-documents-count";

template <typename Clock, typename Duration>
std::int64_t TimeStampToMillisecondsFromNow(
    std::chrono::time_point<Clock, Duration> time) {
  const auto diff = (std::is_same_v<Clock, std::chrono::steady_clock>
                         ? utils::datetime::SteadyNow()
                         : Clock::now()) -
                    time;
  return std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
}

void CombineStatistics(const impl::UpdateStatistics& a,
                       const impl::UpdateStatistics& b,
                       impl::UpdateStatistics& result) {
  result.update_attempt_count =
      a.update_attempt_count.Load() + b.update_attempt_count.Load();
  result.update_no_changes_count =
      a.update_no_changes_count.Load() + b.update_no_changes_count.Load();
  result.update_failures_count =
      a.update_failures_count.Load() + b.update_failures_count.Load();
  result.documents_read_count =
      a.documents_read_count.Load() + b.documents_read_count.Load();
  result.documents_parse_failures =
      a.documents_parse_failures.Load() + b.documents_parse_failures.Load();

  result.last_update_start_time = std::max(a.last_update_start_time.load(),
                                           b.last_update_start_time.load());
  result.last_successful_update_start_time =
      std::max(a.last_successful_update_start_time.load(),
               b.last_successful_update_start_time.load());
  result.last_update_duration =
      std::max(a.last_update_duration.load(), b.last_update_duration.load());
}

}  // namespace

namespace impl {

void DumpMetric(utils::statistics::Writer& writer,
                const UpdateStatistics& stats) {
  // Note about sensor duplication with 'v2' suffix:
  // We have to duplicate metrics with different sensor name to change
  // their type to RATE. Unfortunatelly, we can't change existing metrics
  // because it will break dashboards/alerts for all current users.
  if (auto update = writer["update"]) {
    update["attempts_count"] = stats.update_attempt_count.Load().value;
    update["no_changes_count"] = stats.update_no_changes_count.Load().value;
    update["failures_count"] = stats.update_failures_count.Load().value;
    // v2 - please see note above
    update["attempts_count.v2"] = stats.update_attempt_count;
    update["no_changes_count.v2"] = stats.update_no_changes_count;
    update["failures_count.v2"] = stats.update_failures_count;
  }

  if (auto documents = writer["documents"]) {
    documents["read_count"] = stats.documents_read_count.Load().value;
    documents["parse_failures"] = stats.documents_parse_failures.Load().value;
    // v2 - please see note above
    documents["read_count.v2"] = stats.documents_read_count;
    documents["parse_failures.v2"] = stats.documents_parse_failures;
  }

  if (auto age = writer["time"]) {
    age["time-from-last-update-start-ms"] =
        TimeStampToMillisecondsFromNow(stats.last_update_start_time.load());
    age["time-from-last-successful-start-ms"] = TimeStampToMillisecondsFromNow(
        stats.last_successful_update_start_time.load());
    age["last-update-duration-ms"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            stats.last_update_duration.load())
            .count();
  }
}

void DumpMetric(utils::statistics::Writer& writer, const Statistics& stats) {
  const auto& full = stats.full_update;
  const auto& incremental = stats.incremental_update;
  UpdateStatistics any;
  cache::CombineStatistics(full, incremental, any);

  writer[cache::kStatisticsNameFull] = full;
  writer[cache::kStatisticsNameIncremental] = incremental;
  writer[cache::kStatisticsNameAny] = any;

  writer[cache::kStatisticsNameCurrentDocumentsCount] =
      stats.documents_current_count;
}

}  // namespace impl

UpdateStatisticsScope::UpdateStatisticsScope(impl::Statistics& stats,
                                             cache::UpdateType type)
    : stats_(stats),
      update_stats_(type == cache::UpdateType::kIncremental
                        ? stats.incremental_update
                        : stats.full_update),
      update_start_time_(utils::datetime::SteadyNow()) {
  update_stats_.last_update_start_time = update_start_time_;
  ++update_stats_.update_attempt_count;
}

UpdateStatisticsScope::~UpdateStatisticsScope() {
  if (state_ == impl::UpdateState::kNotFinished) {
    FinishWithError();
  }
}

impl::UpdateState UpdateStatisticsScope::GetState(utils::InternalTag) const {
  return state_;
}

void UpdateStatisticsScope::Finish(std::size_t total_documents_count) {
  stats_.documents_current_count = total_documents_count;
  DoFinish(impl::UpdateState::kSuccess);
}

void UpdateStatisticsScope::FinishNoChanges() {
  ++update_stats_.update_no_changes_count;
  DoFinish(impl::UpdateState::kSuccess);
}

void UpdateStatisticsScope::FinishWithError() {
  ++update_stats_.update_failures_count;
  DoFinish(impl::UpdateState::kFailure);
}

void UpdateStatisticsScope::IncreaseDocumentsReadCount(std::size_t add) {
  update_stats_.documents_read_count += utils::statistics::Rate{add};
}

void UpdateStatisticsScope::IncreaseDocumentsParseFailures(std::size_t add) {
  update_stats_.documents_parse_failures += utils::statistics::Rate{add};
}

void UpdateStatisticsScope::DoFinish(impl::UpdateState new_state) {
  UASSERT(new_state != impl::UpdateState::kNotFinished);
  // TODO Some production caches call Finish multiple times. We should fix those
  //  and add an UASSERT here.
  if (state_ != impl::UpdateState::kNotFinished) return;

  const auto update_stop_time = utils::datetime::SteadyNow();
  if (new_state == impl::UpdateState::kSuccess) {
    update_stats_.last_successful_update_start_time = update_start_time_;
  }
  update_stats_.last_update_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(update_stop_time -
                                                            update_start_time_);

  state_ = new_state;
}

}  // namespace cache

USERVER_NAMESPACE_END
