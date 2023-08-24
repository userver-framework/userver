#include <storages/mongo/stats.hpp>

#include <exception>
#include <tuple>

#include <boost/functional/hash.hpp>

#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::stats {
namespace {

ErrorType ToErrorType(MongoError::Kind mongo_error_kind) {
  switch (mongo_error_kind) {
    case MongoError::Kind::kNetwork:
      return ErrorType::kNetwork;
    case MongoError::Kind::kClusterUnavailable:
      return ErrorType::kClusterUnavailable;
    case MongoError::Kind::kIncompatibleServer:
      return ErrorType::kBadServerVersion;
    case MongoError::Kind::kAuthentication:
      return ErrorType::kAuthFailure;
    case MongoError::Kind::kInvalidQueryArgument:
      return ErrorType::kBadQueryArgument;
    case MongoError::Kind::kServer:
      return ErrorType::kServer;
    case MongoError::Kind::kWriteConcern:
      return ErrorType::kWriteConcern;
    case MongoError::Kind::kDuplicateKey:
      return ErrorType::kDuplicateKey;

    case MongoError::Kind::kNoError:
    case MongoError::Kind::kQuery:
    case MongoError::Kind::kOther:
      return ErrorType::kOther;
  }
  UINVARIANT(false, "Unexpected MongoError::Kind");
}

template <typename Rep, typename Period>
auto GetMilliseconds(const std::chrono::duration<Rep, Period>& duration) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
}

}  // namespace

void OperationStatisticsItem::Account(ErrorType error_type) noexcept {
  ++counters[static_cast<std::size_t>(error_type)];
}

void OperationStatisticsItem::Reset() {
  for (auto& counter : counters) {
    ResetMetric(counter);
  }
  timings.Reset();
}

Rate OperationStatisticsItem::GetCounter(ErrorType error_type) const noexcept {
  return counters[static_cast<std::size_t>(error_type)].Load();
}

Rate OperationStatisticsItem::GetTotalQueries() const noexcept {
  utils::statistics::Rate result;
  for (const auto& counter : counters) {
    result += counter.Load();
  }
  return result;
}

std::string_view ToString(ErrorType type) {
  using Type = ErrorType;
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

    case Type::kCancelled:
      return "cancelled";
    case Type::kPoolOverload:
      return "pool-overload";

    case Type::kErrorTypesCount:
      UINVARIANT(false, "Unexpected kErrorTypesCount");
  }

  UINVARIANT(false, "Unexpected type");
}

std::string_view ToString(OpType type) {
  using Type = OpType;
  switch (type) {
    case Type::kInvalid:
      UINVARIANT(false, "Unexpected OpType::kInvalid");
    case Type::kCount:
      return "count";
    case Type::kCountApprox:
      return "count-approx";
    case Type::kFind:
      return "find";
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
    case Type::kAggregate:
      return "aggregate";
    case Type::kDrop:
      return "drop";
  }

  UINVARIANT(false, "Unexpected type");
}

bool OperationKey::operator==(const OperationKey& other) const noexcept {
  return op_type == other.op_type;
}

PoolConnectStatistics::PoolConnectStatistics()
    : ping(utils::MakeSharedRef<OperationStatisticsItem>()) {}

OperationStopwatch::OperationStopwatch(
    std::shared_ptr<OperationStatisticsItem> stats_ptr, std::string&& label)
    : stats_item_(std::move(stats_ptr)),
      scope_time_(
          tracing::Span::CurrentSpan().CreateScopeTime(std::move(label))) {}

OperationStopwatch::OperationStopwatch(
    std::shared_ptr<OperationStatisticsItem> stats_ptr)
    : OperationStopwatch(std::move(stats_ptr), "operation") {}

OperationStopwatch::~OperationStopwatch() { Account(ErrorType::kOther); }

void OperationStopwatch::AccountSuccess() { Account(ErrorType::kSuccess); }

void OperationStopwatch::AccountError(MongoError::Kind kind) {
  Account(ToErrorType(kind));
}

void OperationStopwatch::Discard() {
  stats_item_.reset();
  scope_time_.Discard();
}

void OperationStopwatch::Account(ErrorType error_type) noexcept {
  const auto stats_item = std::exchange(stats_item_, nullptr);
  if (!stats_item) return;

  try {
    stats_item->Account(error_type);
    auto ms = GetMilliseconds(scope_time_.Reset());
    UASSERT(ms >= 0);
    stats_item->timings.GetCurrentCounter().Account(ms);
    stats_item->timings_sum += utils::statistics::Rate{
        static_cast<utils::statistics::Rate::ValueType>(ms)};
  } catch (const std::exception&) {
    // ignore
  }
}

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

USERVER_NAMESPACE_END

std::size_t
std::hash<USERVER_NAMESPACE::storages::mongo::stats::OperationKey>::operator()(
    USERVER_NAMESPACE::storages::mongo::stats::OperationKey value) const {
  return static_cast<std::size_t>(value.op_type);
}
