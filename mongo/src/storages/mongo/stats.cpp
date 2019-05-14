#include <storages/mongo/stats.hpp>

#include <exception>

namespace storages::mongo::stats {
namespace {

OperationStatisticsItem::ErrorType ToErrorType(
    MongoError::Kind mongo_error_kind) {
  switch (mongo_error_kind) {
    case MongoError::Kind::kNetwork:
      return OperationStatisticsItem::ErrorType::kNetwork;
    case MongoError::Kind::kClusterUnavailable:
      return OperationStatisticsItem::ErrorType::kNetwork;
    case MongoError::Kind::kIncompatibleServer:
      return OperationStatisticsItem::ErrorType::kBadServerVersion;
    case MongoError::Kind::kAuthentication:
      return OperationStatisticsItem::ErrorType::kAuthFailure;
    case MongoError::Kind::kInvalidQueryArgument:
      return OperationStatisticsItem::ErrorType::kBadQueryArgument;
    case MongoError::Kind::kServer:
      return OperationStatisticsItem::ErrorType::kServer;
    case MongoError::Kind::kWriteConcern:
      return OperationStatisticsItem::ErrorType::kWriteConcern;
    case MongoError::Kind::kDuplicateKey:
      return OperationStatisticsItem::ErrorType::kDuplicateKey;

    case MongoError::Kind::kNoError:
    case MongoError::Kind::kQuery:
    case MongoError::Kind::kOther:;  // other
  }
  return OperationStatisticsItem::ErrorType::kOther;
}

}  // namespace

void OperationStatisticsItem::Reset() {
  for (auto& counter : counters) counter = 0;
  timings.Reset();
}

void ReadOperationStatistics::Reset() {
  for (auto& item : items) item.Reset();
}

void WriteOperationStatistics::Reset() {
  for (auto& item : items) item.Reset();
}

void PoolConnectStatistics::Reset() {
  requested = 0;
  created = 0;
  closed = 0;
  overload = 0;
  for (auto& item : items) item.Reset();
  request_timings.Reset();
  queue_wait_timings.Reset();
}

StopwatchImpl::StopwatchImpl() noexcept
    : start_(std::chrono::steady_clock::now()) {}

uint32_t StopwatchImpl::ElapsedMs() const noexcept {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start_)
      .count();
}

template <typename OpStats>
OperationStopwatch<OpStats>::OperationStopwatch(
    std::shared_ptr<Aggregator<OpStats>> stats_agg,
    typename OpStats::OpType op_type)
    : stats_agg_(std::move(stats_agg)), op_type_(op_type) {}

template <typename OpStats>
OperationStopwatch<OpStats>::~OperationStopwatch() {
  if (stats_agg_) Account(OperationStatisticsItem::ErrorType::kOther);
}

template <typename OpStats>
void OperationStopwatch<OpStats>::AccountSuccess() {
  Account(OperationStatisticsItem::ErrorType::kSuccess);
}

template <typename OpStats>
void OperationStopwatch<OpStats>::AccountError(MongoError::Kind kind) {
  Account(ToErrorType(kind));
}

template <typename OpStats>
void OperationStopwatch<OpStats>::Account(
    OperationStatisticsItem::ErrorType error_type) noexcept {
  const auto stats_agg = std::exchange(stats_agg_, nullptr);
  if (!stats_agg) return;

  try {
    auto& stats_item = stats_agg->GetCurrentCounter().items[op_type_];
    ++stats_item.counters[error_type];
    stats_item.timings.Account(stopwatch_.ElapsedMs());
  } catch (const std::exception&) {
    // ignore
  }
}

// explicit instantiations
template class OperationStopwatch<ReadOperationStatistics>;
template class OperationStopwatch<WriteOperationStatistics>;
template class OperationStopwatch<PoolConnectStatistics>;

PoolRequestStopwatch::PoolRequestStopwatch(
    std::shared_ptr<Aggregator<PoolConnectStatistics>> stats_agg)
    : stats_agg_(std::move(stats_agg)) {}

PoolRequestStopwatch::~PoolRequestStopwatch() {
  try {
    auto& stats = stats_agg_->GetCurrentCounter();
    ++stats.requested;
    stats.request_timings.Account(stopwatch_.ElapsedMs());
  } catch (const std::exception&) {
    // ignore
  }
}

PoolQueueStopwatch::PoolQueueStopwatch(
    std::shared_ptr<Aggregator<PoolConnectStatistics>> stats_agg)
    : stats_agg_(std::move(stats_agg)) {}

PoolQueueStopwatch::~PoolQueueStopwatch() {
  if (stats_agg_) Stop();
}

void PoolQueueStopwatch::Stop() noexcept {
  const auto stats_agg = std::exchange(stats_agg_, nullptr);
  if (!stats_agg) return;

  try {
    auto& stats = stats_agg->GetCurrentCounter();
    stats.queue_wait_timings.Account(stopwatch_.ElapsedMs());
  } catch (const std::exception&) {
    // ignore
  }
}

}  // namespace storages::mongo::stats
