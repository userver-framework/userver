#pragma once

/// @file userver/storages/rocks/detail/iterator_impl.hpp
/// @brief @copybrief storages::rocks::detail::IteratorImpl

#include <memory>
#include <string>
#include <string_view>

namespace rocksdb {
class Iterator;
class Snapshot;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks::detail {

class DbImpl;

class IteratorImpl {
public:
    IteratorImpl(const std::shared_ptr<DbImpl>& db, const std::shared_ptr<const rocksdb::Snapshot>& snapshot,
            std::unique_ptr<rocksdb::Iterator> iterator) noexcept;
    virtual ~IteratorImpl();

    IteratorImpl(IteratorImpl&&) noexcept;
    IteratorImpl& operator=(IteratorImpl&&) noexcept;

    [[nodiscard]] bool Valid() const noexcept;
    void SeekToFirst();
    void SeekToLast();
    void Seek(std::string_view slice);
    void SeekForPrev(std::string_view slice);
    void Next();
    void Prev();

    [[nodiscard]] std::string Key() const;
    [[nodiscard]] std::string Value() const;

private:
    std::shared_ptr<DbImpl> db_impl_;
    std::shared_ptr<const rocksdb::Snapshot> snapshot_;
    std::unique_ptr<rocksdb::Iterator> iterator_;
};

}  // namespace storages::rocks::detail

USERVER_NAMESPACE_END
