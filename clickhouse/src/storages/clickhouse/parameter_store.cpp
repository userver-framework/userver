#include <userver/storages/clickhouse/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

bool ParameterStore::IsEmpty() const { return parameters_.size(); }

size_t ParameterStore::Size() const { return parameters_.size(); }

const fmt::dynamic_format_arg_store<fmt::format_context>& ParameterStore::GetParameters() const { return parameters_; }

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
