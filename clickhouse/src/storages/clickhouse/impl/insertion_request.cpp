#include <userver/storages/clickhouse/impl/insertion_request.hpp>

#include <clickhouse/block.h>

#include <storages/clickhouse/impl/block_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

InsertionRequest::InsertionRequest(
    const std::string& table_name,
    const std::vector<std::string_view>& column_names)
    : table_name_{table_name},
      column_names_{column_names},
      block_{std::make_unique<impl::BlockWrapper>(
          impl::clickhouse_cpp::Block{column_names_.size(), 0})} {}

InsertionRequest::InsertionRequest(InsertionRequest&&) noexcept = default;

InsertionRequest::~InsertionRequest() = default;

const std::string& InsertionRequest::GetTableName() const {
  return table_name_;
}

const impl::BlockWrapper& InsertionRequest::GetBlock() const { return *block_; }

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
