#include <userver/storages/rocks/cursor.hpp>

#include <rocksdb/iterator.h>

#include <userver/engine/async.hpp>
#include <userver/storages/rocks/exception.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

Cursor::Cursor(
    std::shared_ptr<rocksdb::DB> db,
    engine::TaskProcessor& tp,
    const rocksdb::Snapshot* snap,
    std::string prefix
)
    : db_(std::move(db)),
      tp_(&tp),
      snap_(snap),
      prefix_(std::move(prefix))
{}

Cursor::Cursor(Cursor&& other) noexcept
    : db_(std::move(other.db_)),
      tp_(other.tp_),
      snap_(other.snap_),
      it_(std::move(other.it_)),
      prefix_(std::move(other.prefix_)),
      done_(other.done_)
{
    other.snap_ = nullptr;
    other.done_ = true;
}

Cursor& Cursor::operator=(Cursor&& other) noexcept {
    if (this != &other) {
        if (snap_ != nullptr) {
            db_->ReleaseSnapshot(snap_);
        }
        db_ = std::move(other.db_);
        tp_ = other.tp_;
        snap_ = other.snap_;
        it_ = std::move(other.it_);
        prefix_ = std::move(other.prefix_);
        done_ = other.done_;
        other.snap_ = nullptr;
        other.done_ = true;
    }
    return *this;
}

Cursor::~Cursor() {
    it_.reset();
    if (snap_ != nullptr) {
        db_->ReleaseSnapshot(snap_);
    }
}

std::vector<KeyValue> Cursor::FetchBatch(std::size_t batch_size) {
    if (done_) {
        return {};
    }

    return engine::AsyncNoTracing(
               *tp_,
               [this, batch_size] {
                   if (it_ == nullptr) {
                       rocksdb::ReadOptions ro;
                       ro.snapshot = snap_;
                       it_.reset(db_->NewIterator(ro));
                       if (prefix_.empty()) {
                           it_->SeekToFirst();
                       } else {
                           it_->Seek(prefix_);
                       }
                   }

                   std::vector<KeyValue> batch;
                   batch.reserve(batch_size);

                   for (std::size_t i = 0; i < batch_size && it_->Valid(); ++i, it_->Next()) {
                       const rocksdb::Slice k = it_->key();
                       const std::string_view key_sv{k.data(), k.size()};
                       if (!prefix_.empty() && !key_sv.starts_with(prefix_)) {
                           done_ = true;
                           break;
                       }
                       batch.push_back(KeyValue{
                           .key = k.ToString(),
                           .value = it_->value().ToString(),
                       });
                   }

                   if (!it_->status().ok()) {
                       throw RequestFailedException("Cursor::FetchBatch", it_->status().ToString());
                   }
                   if (batch.size() < batch_size) {
                       done_ = true;
                   }
                   return batch;
               }
    ).Get();
}

}  // namespace storages::rocks

USERVER_NAMESPACE_END
