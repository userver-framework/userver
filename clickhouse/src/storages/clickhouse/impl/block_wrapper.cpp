#include "block_wrapper.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

BlockWrapper::BlockWrapper(clickhouse_cpp::Block&& block) : native_{block} {}

clickhouse_cpp::ColumnRef BlockWrapper::At(size_t ind) const {
  return native_[ind];
}

size_t BlockWrapper::GetColumnsCount() const {
  return native_.GetColumnCount();
}

size_t BlockWrapper::GetRowsCount() const { return native_.GetRowCount(); }

void BlockWrapper::AppendColumn(std::string_view name,
                                const clickhouse_cpp::ColumnRef& column) {
  native_.AppendColumn(std::string{name}, column);
}

const clickhouse_cpp::Block& BlockWrapper::GetNative() const { return native_; }

void BlockWrapperDeleter::operator()(BlockWrapper* ptr) const noexcept {
  std::default_delete<BlockWrapper>{}(ptr);
}

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
