#pragma once

/// @file userver/storages/rocks/write_batch.hpp
/// @brief @copybrief storages::rocks::WriteBatch

#include <cstddef>
#include <string_view>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/storages/rocks/column_family.hpp>

namespace rocksdb {
class WriteBatch;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

class Db;

class WriteBatch final {
public:
    WriteBatch(std::size_t reserved_bytes = 0, std::size_t max_bytes = 0);
    ~WriteBatch();

    void Put(std::string_view key, std::string_view value);
    void Put(ColumnFamilyHandle column_family, std::string_view key, std::string_view value);

    void Delete(std::string_view key);
    void Delete(ColumnFamilyHandle column_family, std::string_view key);

private:
    friend class Db;
    rocksdb::WriteBatch& GetPimpl();

    static constexpr std::size_t kImplSize = 160;
    static constexpr std::size_t kImplAlign = 16;
    utils::FastPimpl<rocksdb::WriteBatch, kImplSize, kImplAlign> write_batch_impl_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
