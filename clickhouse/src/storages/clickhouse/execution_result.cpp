#include <userver/storages/clickhouse/execution_result.hpp>

#include <storages/clickhouse/impl/block_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

ExecutionResult::ExecutionResult(impl::BlockWrapperPtr block)
    : block_{std::move(block)} {}

ExecutionResult::ExecutionResult(ExecutionResult&&) noexcept = default;

ExecutionResult::~ExecutionResult() = default;

size_t ExecutionResult::GetColumnsCount() const {
  return block_->GetColumnsCount();
}

size_t ExecutionResult::GetRowsCount() const { return block_->GetRowsCount(); }

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
