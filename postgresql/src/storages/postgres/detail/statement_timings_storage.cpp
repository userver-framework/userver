#include "statement_timings_storage.hpp"

#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

namespace {
constexpr size_t kDefaultEventsQueueSize = 1000;
}

StatementTimingsStorage::StatementTimingsStorage(
    const StatementMetricsSettings& settings)
    : settings_{settings},
      enabled_{settings.max_statements != 0},
      data_{
          CreateStorageData(settings.max_statements, kDefaultEventsQueueSize)} {
  data_.consumer_task =
      utils::Async("pg_timings_events_consumer", [this]() { ProcessEvents(); });
}

StatementTimingsStorage::~StatementTimingsStorage() {
  if (data_.consumer_task.IsValid()) {
    data_.consumer_task.SyncCancel();
  }
}

void StatementTimingsStorage::Account(const std::string& statement_name,
                                      std::size_t duration_ms) const {
  if (!IsEnabled()) return;

  const auto producer = data_.events_queue->GetProducer();

  [[maybe_unused]] const auto success = producer.PushNoblock(
      std::make_unique<StatementEvent>(statement_name, duration_ms));
}

std::unordered_map<std::string, StatementTimingsStorage::Percentile>
StatementTimingsStorage::GetTimingsPercentiles() const {
  if (!IsEnabled()) return {};

  auto locked_ptr = data_.timings->SharedLock();
  const auto& timings = *locked_ptr;

  std::unordered_map<std::string, StatementTimingsStorage::Percentile> result;
  result.reserve(timings.GetSize());

  timings.VisitAll(
      [&result](const std::string& key,
                const std::unique_ptr<StatementTimingsStorage::RecentPeriod>&
                    recent_period) {
        result.emplace(key, recent_period->GetStatsForPeriod());
      });

  return result;
}

void StatementTimingsStorage::SetSettings(
    const StatementMetricsSettings& settings) {
  const auto settings_ptr = settings_.Read();
  if (*settings_ptr == settings) return;

  const auto enabled = settings.max_statements != 0;
  if (enabled) {
    // we don't have any operations that hold this lock for long,
    // so it should be ok
    auto locked_ptr = data_.timings->UniqueLock();
    auto& timings = *locked_ptr;

    timings.SetMaxSize(settings.max_statements);
  }

  enabled_ = enabled;
  settings_.Assign(settings);
}

void StatementTimingsStorage::WaitForExhaustion() const {
  while (data_.events_queue->GetSizeApproximate()) {
    engine::SleepFor(std::chrono::milliseconds{2});
  }
}

bool StatementTimingsStorage::IsEnabled() const { return enabled_; }

void StatementTimingsStorage::ProcessEvents() {
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

void StatementTimingsStorage::AccountEvent(EventPtr event_ptr) const {
  const auto& name = event_ptr->statement_name;
  const auto duration = event_ptr->duration_ms;

  auto locked_ptr = data_.timings->UniqueLock();
  auto& data = *locked_ptr;

  auto* timing_ptr = data.Get(name);
  if (!timing_ptr) {
    data.Put(name, std::make_unique<RecentPeriod>());
    timing_ptr = data.Get(name);
  }
  UASSERT(timing_ptr);
  (*timing_ptr)->GetCurrentCounter().Account(duration);
}

StatementTimingsStorage::StorageData StatementTimingsStorage::CreateStorageData(
    size_t max_map_size, size_t max_queue_size) {
  // We create storage and queue unconditionally, because
  // it gets complicated to process config updates otherwise
  if (!max_map_size) max_map_size = 1;

  auto queue_ptr = Queue::Create(max_queue_size);

  return StatementTimingsStorage::StorageData{
      std::make_unique<Storage>(max_map_size),
      queue_ptr,
      {},
      queue_ptr->GetProducer()};
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
