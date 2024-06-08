#include "statement_stats_storage.hpp"

#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

namespace {
constexpr size_t kDefaultEventsQueueSize = 1000;
}

StatementStatsStorage::StatementStatsStorage(
    const StatementMetricsSettings& settings)
    : settings_{settings},
      enabled_{settings.max_statements != 0},
      data_{
          CreateStorageData(settings.max_statements, kDefaultEventsQueueSize)} {
  data_.consumer_task =
      utils::Async("pg_stats_events_consumer", [this]() { ProcessEvents(); });
}

StatementStatsStorage::~StatementStatsStorage() {
  if (data_.consumer_task.IsValid()) {
    data_.consumer_task.SyncCancel();
  }
}

void StatementStatsStorage::Account(const std::string& statement_name,
                                    std::size_t duration_ms,
                                    ExecutionResult result) const {
  if (!IsEnabled()) return;

  const auto producer = data_.events_queue->GetProducer();

  [[maybe_unused]] const auto success = producer.PushNoblock(
      std::make_unique<StatementEvent>(statement_name, duration_ms, result));
}

std::unordered_map<std::string, StatementStatistics>
StatementStatsStorage::GetStatementsStats() const {
  if (!IsEnabled()) return {};

  auto locked_ptr = data_.stats->SharedLock();
  const auto& stats = *locked_ptr;

  std::unordered_map<std::string, StatementStatistics> result;
  result.reserve(stats.GetSize());

  stats.VisitAll([&result](const std::string& key,
                           const std::unique_ptr<StoredStatementStats>& value) {
    result.emplace(key, StatementStatistics{value->timings.GetStatsForPeriod(),
                                            value->executed, value->errors});
  });

  return result;
}

void StatementStatsStorage::SetSettings(
    const StatementMetricsSettings& settings) {
  const auto settings_ptr = settings_.Read();
  if (*settings_ptr == settings) return;

  const auto enabled = settings.max_statements != 0;
  if (enabled) {
    // we don't have any operations that hold this lock for long,
    // so it should be ok
    auto locked_ptr = data_.stats->UniqueLock();
    auto& stats = *locked_ptr;

    stats.SetMaxSize(settings.max_statements);
  }

  enabled_ = enabled;
  settings_.Assign(settings);
}

void StatementStatsStorage::WaitForExhaustion() const {
  while (data_.events_queue->GetSizeApproximate()) {
    engine::SleepFor(std::chrono::milliseconds{2});
  }
}

bool StatementStatsStorage::IsEnabled() const { return enabled_; }

void StatementStatsStorage::ProcessEvents() {
  const auto consumer = data_.events_queue->GetConsumer();

  EventPtr event_ptr;
  while (!engine::current_task::IsCancelRequested()) {
    if (!consumer.Pop(event_ptr)) {
      break;
    }
    UASSERT(event_ptr);

    AccountEvent(std::move(event_ptr));
  }
}

void StatementStatsStorage::AccountEvent(EventPtr event_ptr) const {
  const auto& name = event_ptr->statement_name;
  const auto duration = event_ptr->duration_ms;
  const auto result = event_ptr->result;

  auto locked_ptr = data_.stats->UniqueLock();
  auto& data = *locked_ptr;

  auto* stats_ptr = data.Get(name);
  if (!stats_ptr) {
    data.Put(name, std::make_unique<StoredStatementStats>());
    stats_ptr = data.Get(name);
  }
  UASSERT(stats_ptr);

  auto& stats_ref = *stats_ptr;
  switch (result) {
    case ExecutionResult::kSuccess:
      stats_ref->timings.GetCurrentCounter().Account(duration);
      stats_ref->executed.Add({1});
      break;
    case ExecutionResult::kError:
      stats_ref->errors.Add({1});
      break;
  }
}

StatementStatsStorage::StorageData StatementStatsStorage::CreateStorageData(
    size_t max_map_size, size_t max_queue_size) {
  // We create storage and queue unconditionally, because
  // it gets complicated to process config updates otherwise
  if (!max_map_size) max_map_size = 1;

  auto queue_ptr = Queue::Create(max_queue_size);

  return StatementStatsStorage::StorageData{
      std::make_unique<Storage>(max_map_size),
      queue_ptr,
      {},
      queue_ptr->GetProducer()};
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
