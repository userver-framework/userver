#include <userver/storages/rocks/write_batch.hpp>
#include <userver/storages/rocks/exception.hpp>
#include <rocksdb/write_batch.h>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

WriteBatch::WriteBatch(std::size_t reserved_bytes, std::size_t max_bytes)
    : write_batch_impl_{reserved_bytes, max_bytes} {}

WriteBatch::~WriteBatch() = default;

void WriteBatch::Put(std::string_view key, std::string_view value) {
    auto status = write_batch_impl_->Put(key, value);
    detail::CheckStatus(status, "Put");
}

void WriteBatch::Put(ColumnFamilyHandle column_family, std::string_view key, std::string_view value) {
    auto status = write_batch_impl_->Put(column_family, key, value);
    detail::CheckStatus(status, "Put");
}

void WriteBatch::Delete(std::string_view key) {
    auto status = write_batch_impl_->Delete(key);
    detail::CheckStatus(status, "Delete");
}

void WriteBatch::Delete(ColumnFamilyHandle column_family, std::string_view key) {
    auto status = write_batch_impl_->Delete(column_family, key);
    detail::CheckStatus(status, "Delete");
}

rocksdb::WriteBatch& WriteBatch::GetPimpl() {
    return *write_batch_impl_;
}

}  // namespace storages::rocks

USERVER_NAMESPACE_END
