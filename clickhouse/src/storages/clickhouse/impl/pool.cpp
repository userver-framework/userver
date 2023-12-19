#include <userver/storages/clickhouse/impl/pool.hpp>

#include <userver/storages/clickhouse/query.hpp>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

#include <storages/clickhouse/impl/connection.hpp>
#include <storages/clickhouse/impl/connection_ptr.hpp>
#include <storages/clickhouse/impl/pool_impl.hpp>
#include <storages/clickhouse/impl/tracing_tags.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

namespace {

tracing::Span PrepareExecutionSpan(const std::string& scope,
                                   const std::string& db_instance) {
  tracing::Span span{scope};
  span.AddTag(tracing::kDatabaseInstance, db_instance);

  return span;
}

}  // namespace

Pool::Pool(clients::dns::Resolver& resolver, PoolSettings&& settings)
    : impl_{std::make_shared<impl::PoolImpl>(resolver, std::move(settings))} {
  impl_->StartMaintenance();
}

Pool::~Pool() = default;

ExecutionResult Pool::Execute(OptionalCommandControl optional_cc,
                              const Query& query) const {
  auto conn_ptr = impl_->Acquire();

  auto span = PrepareExecutionSpan(impl::scopes::kQuery, impl_->GetHostName());
  query.FillSpanTags(span);

  const auto timer = impl_->GetExecuteTimer();
  return conn_ptr->Execute(optional_cc, query);
}

void Pool::Insert(OptionalCommandControl optional_cc,
                  const InsertionRequest& request) const {
  auto conn_ptr = impl_->Acquire();

  auto span = PrepareExecutionSpan(impl::scopes::kInsert, impl_->GetHostName());

  const auto timer = impl_->GetInsertTimer();
  conn_ptr->Insert(optional_cc, request);
}

void Pool::WriteStatistics(
    USERVER_NAMESPACE::utils::statistics::Writer& writer) const {
  writer.ValueWithLabels(impl_->GetStatistics(),
                         {{"clickhouse_instance", impl_->GetHostName()}});
}

bool Pool::IsAvailable() const { return impl_->IsAvailable(); }

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
