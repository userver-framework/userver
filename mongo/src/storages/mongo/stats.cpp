#include <storages/mongo/stats.hpp>

#include <exception>

#include <tracing/span.hpp>

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

template <typename Rep, typename Period>
auto GetMilliseconds(const std::chrono::duration<Rep, Period>& duration) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
}

}  // namespace

OperationStatisticsItem::OperationStatisticsItem() {
  // success counter is always considered used
  for (size_t i = 1; i < counters.size(); ++i) {
    counters[i] = kUnusedCounter;
  }
}

OperationStatisticsItem::OperationStatisticsItem(
    const OperationStatisticsItem& other)
    : timings(other.timings) {
  for (size_t i = 0; i < counters.size(); ++i) {
    counters[i] = other.counters[i].Load();
  }
}

void OperationStatisticsItem::Reset() {
  for (auto& counter : counters) {
    if (counter != kUnusedCounter) counter = 0;
  }
  timings.Reset();
}

std::string ToString(OperationStatisticsItem::ErrorType type) {
  using Type = OperationStatisticsItem::ErrorType;
  switch (type) {
    case Type::kSuccess:
      return "success";
    case Type::kNetwork:
      return "network";
    case Type::kClusterUnavailable:
      return "cluster-unavailable";
    case Type::kBadServerVersion:
      return "server-version";
    case Type::kAuthFailure:
      return "auth-failure";
    case Type::kBadQueryArgument:
      return "bad-query-arg";
    case Type::kDuplicateKey:
      return "duplicate-key";
    case Type::kWriteConcern:
      return "write-concern";
    case Type::kServer:
      return "server-error";
    case Type::kOther:
      return "other";

    case Type::kErrorTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

std::string ToString(ReadOperationStatistics::OpType type) {
  using Type = ReadOperationStatistics::OpType;
  switch (type) {
    case Type::kCount:
      return "count";
    case Type::kCountApprox:
      return "count-approx";
    case Type::kFind:
      return "find";
    case Type::kGetMore:
      return "getmore";
  }
}

std::string ToString(WriteOperationStatistics::OpType type) {
  using Type = WriteOperationStatistics::OpType;
  switch (type) {
    case Type::kInsertOne:
      return "insert-one";
    case Type::kInsertMany:
      return "insert-many";
    case Type::kReplaceOne:
      return "replace-one";
    case Type::kUpdateOne:
      return "update-one";
    case Type::kUpdateMany:
      return "update-many";
    case Type::kDeleteOne:
      return "delete-one";
    case Type::kDeleteMany:
      return "delete-many";
    case Type::kFindAndModify:
      return "find-and-modify";
    case Type::kFindAndRemove:
      return "find-and-remove";
    case Type::kBulk:
      return "bulk";
  }
}

PoolConnectStatistics::PoolConnectStatistics() {
  for (auto& item : items) {
    item = std::make_shared<Aggregator<OperationStatisticsItem>>();
  }
}

std::string ToString(PoolConnectStatistics::OpType type) {
  using Type = PoolConnectStatistics::OpType;
  switch (type) {
    case Type::kPing:
      return "ping";

    case Type::kOpTypesCount:
      UASSERT_MSG(false, "invalid type");
      throw std::logic_error("invalid type");
  }
}

template <typename OpStats>
OperationStopwatch<OpStats>::OperationStopwatch()
    : scope_time_(tracing::Span::CurrentSpan().CreateScopeTime()) {}

template <typename OpStats>
OperationStopwatch<OpStats>::OperationStopwatch(
    const std::shared_ptr<OpStats>& stats_ptr, typename OpStats::OpType op_type)
    : OperationStopwatch<OpStats>() {
  Reset(stats_ptr, op_type);
}

template <typename OpStats>
OperationStopwatch<OpStats>::~OperationStopwatch() {
  if (stats_item_agg_) Account(OperationStatisticsItem::ErrorType::kOther);
}

template <typename OpStats>
void OperationStopwatch<OpStats>::Reset(
    const std::shared_ptr<OpStats>& stats_ptr,
    typename OpStats::OpType op_type) {
  stats_item_agg_ = stats_ptr->items[op_type];
  scope_time_.Reset(ToString(op_type));
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
void OperationStopwatch<OpStats>::Discard() {
  stats_item_agg_.reset();
  scope_time_.Discard();
}

template <typename OpStats>
void OperationStopwatch<OpStats>::Account(
    OperationStatisticsItem::ErrorType error_type) noexcept {
  const auto stats_item_agg = std::exchange(stats_item_agg_, nullptr);
  if (!stats_item_agg) return;

  try {
    auto& stats_item = stats_item_agg->GetCurrentCounter();
    if (!++stats_item.counters[error_type]) {
      static_assert(OperationStatisticsItem::kUnusedCounter == -1,
                    "increment should reset the unused counter");
      // was not used, adjust
      ++stats_item.counters[error_type];
    }
    stats_item.timings.Account(GetMilliseconds(scope_time_.Reset()));
  } catch (const std::exception&) {
    // ignore
  }
}

// explicit instantiations
template class OperationStopwatch<ReadOperationStatistics>;
template class OperationStopwatch<WriteOperationStatistics>;
template class OperationStopwatch<PoolConnectStatistics>;

ConnectionWaitStopwatch::ConnectionWaitStopwatch(
    std::shared_ptr<PoolConnectStatistics> stats_ptr)
    : stats_ptr_(std::move(stats_ptr)),
      scope_time_(tracing::Span::CurrentSpan().CreateScopeTime("conn-wait")) {}

ConnectionWaitStopwatch::~ConnectionWaitStopwatch() {
  try {
    ++stats_ptr_->requested;
    stats_ptr_->request_timings_agg.GetCurrentCounter().Account(
        GetMilliseconds(scope_time_.Reset()));
  } catch (const std::exception&) {
    // ignore
  }
}

ConnectionThrottleStopwatch::ConnectionThrottleStopwatch(
    std::shared_ptr<PoolConnectStatistics> stats_ptr)
    : stats_ptr_(std::move(stats_ptr)),
      scope_time_(
          tracing::Span::CurrentSpan().CreateScopeTime("conn-throttle")) {}

ConnectionThrottleStopwatch::~ConnectionThrottleStopwatch() {
  if (stats_ptr_) Stop();
}

void ConnectionThrottleStopwatch::Stop() noexcept {
  const auto stats_ptr = std::exchange(stats_ptr_, nullptr);
  if (!stats_ptr) return;

  try {
    stats_ptr->queue_wait_timings_agg.GetCurrentCounter().Account(
        GetMilliseconds(scope_time_.Reset()));
  } catch (const std::exception&) {
    // ignore
  }
}

}  // namespace storages::mongo::stats
