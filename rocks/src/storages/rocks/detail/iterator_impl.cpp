#include <userver/storages/rocks/detail/iterator_impl.hpp>
#include <userver/storages/rocks/detail/db_impl.hpp>
#include <userver/storages/rocks/exception.hpp>
#include <userver/utils/async.hpp>
#include <rocksdb/iterator.h>
#include <rocksdb/snapshot.h>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks::detail {

IteratorImpl::IteratorImpl(const std::shared_ptr<DbImpl>& db, const std::shared_ptr<const rocksdb::Snapshot>& snapshot,
        std::unique_ptr<rocksdb::Iterator> iterator) noexcept :
    db_impl_{db}, snapshot_{snapshot}, iterator_{std::move(iterator)} {}

IteratorImpl::~IteratorImpl() = default;

IteratorImpl::IteratorImpl(IteratorImpl&&) noexcept = default;
IteratorImpl& IteratorImpl::operator=(IteratorImpl&&) noexcept = default;

bool IteratorImpl::Valid() const noexcept {
    return iterator_->Valid();
}

void IteratorImpl::SeekToFirst() {
    auto task =
        engine::AsyncNoSpan(db_impl_->GetTaskProcessor(), [this]() {
            iterator_->SeekToFirst();
            return iterator_->status();
        });
    CheckStatus(task.Get(), "SeekToFirst");
}

void IteratorImpl::SeekToLast() {
    auto task =
        engine::AsyncNoSpan(db_impl_->GetTaskProcessor(), [this]() {
            iterator_->SeekToLast();
            return iterator_->status();
        });
    CheckStatus(task.Get(), "SeekToLast");
}

void IteratorImpl::Seek(std::string_view slice) {
    auto task =
        engine::AsyncNoSpan(db_impl_->GetTaskProcessor(), [this, slice]() {
            iterator_->Seek(slice);
            return iterator_->status();
        });
    CheckStatus(task.Get(), "Seek");
}

void IteratorImpl::SeekForPrev(std::string_view slice) {
    auto task =
        engine::AsyncNoSpan(db_impl_->GetTaskProcessor(), [this, slice]() {
            iterator_->SeekForPrev(slice);
            return iterator_->status();
        });
    CheckStatus(task.Get(), "SeekForPrev");
}

void IteratorImpl::Next() {
    auto task =
        engine::AsyncNoSpan(db_impl_->GetTaskProcessor(), [this]() {
            iterator_->Next();
            return iterator_->status();
        });
    CheckStatus(task.Get(), "Next");
}

void IteratorImpl::Prev() {
    auto task =
        engine::AsyncNoSpan(db_impl_->GetTaskProcessor(), [this]() {
            iterator_->Prev();
            return iterator_->status();
        });
    CheckStatus(task.Get(), "Prev");
}

std::string IteratorImpl::Key() const {
    // TODO: Throw an exception on !Valid()?
    return iterator_->key().ToString();
}

std::string IteratorImpl::Value() const {
    // TODO: Throw an exception on !Valid()?
    return iterator_->value().ToString();
}

}  // namespace storages::rocks::detail

USERVER_NAMESPACE_END
