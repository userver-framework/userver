#include "connection.hpp"

#include <exception>

#include <clickhouse/block.h>
#include <clickhouse/query.h>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/clickhouse/impl/insertion_request.hpp>
#include <userver/storages/clickhouse/query.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>

#include <storages/clickhouse/impl/block_wrapper.hpp>
#include <storages/clickhouse/impl/native_client_factory.hpp>
#include <storages/clickhouse/impl/tracing_tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

namespace {

using NativeBlock = clickhouse_cpp::Block;

constexpr std::chrono::milliseconds kDefaultExecuteTimeout{750};

void AppendToBlock(NativeBlock& result, const NativeBlock& new_data) {
  if (new_data.GetColumnCount() == 0) return;
  if (result.GetRowCount() == 0) {
    result = new_data;
    return;
  }
  UINVARIANT(result.GetColumnCount() == new_data.GetColumnCount(),
             "Shouldn't happen");

  for (size_t ind = 0; ind < result.GetColumnCount(); ++ind) {
    const auto& result_column = result[ind];
    const auto& new_column = new_data[ind];
    UINVARIANT(result_column->Type()->IsEqual(new_column->Type()),
               "Shouldn't happen");
    result_column->Append(new_column);
  }

  result.RefreshRowCount();
}

engine::Deadline GetDeadline(OptionalCommandControl optional_cc) {
  const auto duration =
      optional_cc.has_value() ? optional_cc->execute : kDefaultExecuteTimeout;

  return engine::Deadline::FromDuration(duration);
}

}  // namespace

class Connection::ConnectionBrokenGuard final {
 public:
  ConnectionBrokenGuard(bool& broken)
      : exceptions_on_enter_{std::uncaught_exceptions()}, broken_{broken} {
    UINVARIANT(!broken_, "Connection is broken");
  }

  ~ConnectionBrokenGuard() {
    if (std::uncaught_exceptions() != exceptions_on_enter_) {
      broken_ = true;
    }
  }

 private:
  const int exceptions_on_enter_;
  bool& broken_;
};

Connection::Connection(clients::dns::Resolver& resolver,
                       const EndpointSettings& endpoint,
                       const AuthSettings& auth,
                       const ConnectionSettings& connection_settings)
    : client_{NativeClientFactory::Create(resolver, endpoint, auth,
                                          connection_settings)} {}

ExecutionResult Connection::Execute(OptionalCommandControl optional_cc,
                                    const Query& query) {
  clickhouse_cpp::Query native_query{query.QueryText()};
  native_query.OnDataCancelable([]([[maybe_unused]] const auto& block) {
    // we must return 'true' if we don't want to cancel query
    return !engine::current_task::ShouldCancel();
  });

  auto& span = tracing::Span::CurrentSpan();
  auto scope = span.CreateScopeTime(scopes::kExec);

  NativeBlock result{};
  native_query.OnData([&result, &scope](const NativeBlock& data) {
    scope.Reset(scopes::kExec);
    AppendToBlock(result, data);
  });

  DoExecute(optional_cc, native_query);

  auto result_ptr = std::make_unique<BlockWrapper>(std::move(result));

  return ExecutionResult{BlockWrapperPtr{result_ptr.release()}};
}

void Connection::Insert(OptionalCommandControl optional_cc,
                        const InsertionRequest& request) {
  const auto& block = request.GetBlock();

  auto guard = GetBrokenGuard();
  client_.Insert(request.GetTableName(), block.GetNative(),
                 GetDeadline(optional_cc));
}

void Connection::Ping() {
  auto guard = GetBrokenGuard();
  client_.Ping(GetDeadline({}));
}

bool Connection::IsBroken() const noexcept { return broken_; }

Connection::ConnectionBrokenGuard Connection::GetBrokenGuard() {
  return ConnectionBrokenGuard{broken_};
}

void Connection::DoExecute(OptionalCommandControl optional_cc,
                           const clickhouse_cpp::Query& query) {
  auto guard = GetBrokenGuard();
  client_.Execute(query, GetDeadline(optional_cc));
}

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
