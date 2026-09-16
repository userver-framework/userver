#include <userver/storages/rocks/transaction.hpp>

#include <exception>

#include <rocksdb/utilities/transaction.h>

#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/rocks/exception.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

Transaction::Transaction(
    std::shared_ptr<rocksdb::DB> db,
    engine::TaskProcessor& tp,
    std::unique_ptr<rocksdb::Transaction> txn
)
    : db_(std::move(db)),
      tp_(&tp),
      txn_(std::move(txn))
{}

Transaction::Transaction(Transaction&& other) noexcept
    : db_(std::move(other.db_)),
      tp_(other.tp_),
      txn_(std::move(other.txn_)),
      done_(other.done_)
{
    other.done_ = true;
}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (!done_ && txn_ != nullptr) {
            txn_->Rollback();
        }
        db_ = std::move(other.db_);
        tp_ = other.tp_;
        txn_ = std::move(other.txn_);
        done_ = other.done_;
        other.done_ = true;
    }
    return *this;
}

Transaction::~Transaction() {
    if (!done_ && txn_ != nullptr) {
        const engine::TaskCancellationBlocker cancellation_blocker;
        try {
            Rollback();
        } catch (const std::exception& exception) {
            LOG_ERROR() << "Failed to roll back RocksDB transaction in destructor: " << exception;
        }
    }
}

void Transaction::Put(std::string_view key, std::string_view value) {
    engine::AsyncNoTracing(*tp_, [this, key, value] {
        const rocksdb::Status status = txn_->Put(key, value);
        if (!status.ok()) {
            throw RequestFailedException("Transaction::Put", status.ToString());
        }
    }).Get();
}

void Transaction::Delete(std::string_view key) {
    engine::AsyncNoTracing(*tp_, [this, key] {
        const rocksdb::Status status = txn_->Delete(key);
        if (!status.ok()) {
            throw RequestFailedException("Transaction::Delete", status.ToString());
        }
    }).Get();
}

std::optional<std::string> Transaction::Get(std::string_view key) {
    return engine::AsyncNoTracing(
               *tp_,
               [this, key]() -> std::optional<std::string> {
                   std::string value;
                   const rocksdb::Status status = txn_->Get(rocksdb::ReadOptions(), key, &value);
                   if (status.IsNotFound()) {
                       return std::nullopt;
                   }
                   if (!status.ok()) {
                       throw RequestFailedException("Transaction::Get", status.ToString());
                   }
                   return value;
               }
    ).Get();
}

std::optional<std::string> Transaction::GetForUpdate(std::string_view key) {
    return engine::AsyncNoTracing(
               *tp_,
               [this, key]() -> std::optional<std::string> {
                   std::string value;
                   const rocksdb::Status status = txn_->GetForUpdate(rocksdb::ReadOptions(), key, &value);
                   if (status.IsNotFound()) {
                       return std::nullopt;
                   }
                   if (status.IsTimedOut()) {
                       throw LockTimeoutException("Transaction::GetForUpdate: lock timed out");
                   }
                   if (!status.ok()) {
                       throw RequestFailedException("Transaction::GetForUpdate", status.ToString());
                   }
                   return value;
               }
    ).Get();
}

void Transaction::Commit() {
    engine::AsyncNoTracing(*tp_, [this] {
        const rocksdb::Status status = txn_->Commit();
        if (status.IsBusy() || status.IsTryAgain()) {
            throw WriteConflictException("Transaction::Commit: write conflict detected");
        }
        if (!status.ok()) {
            throw RequestFailedException("Transaction::Commit", status.ToString());
        }
        done_ = true;
    }).Get();
}

void Transaction::Rollback() {
    engine::AsyncNoTracing(*tp_, [this] {
        const rocksdb::Status status = txn_->Rollback();
        if (!status.ok()) {
            throw RequestFailedException("Transaction::Rollback", status.ToString());
        }
        done_ = true;
    }).Get();
}

void Transaction::SetSavePoint() {
    engine::AsyncNoTracing(*tp_, [this] { txn_->SetSavePoint(); }).Get();
}

void Transaction::RollbackToSavePoint() {
    engine::AsyncNoTracing(*tp_, [this] {
        const rocksdb::Status status = txn_->RollbackToSavePoint();
        if (!status.ok()) {
            throw RequestFailedException("Transaction::RollbackToSavePoint", status.ToString());
        }
    }).Get();
}

}  // namespace storages::rocks

USERVER_NAMESPACE_END
