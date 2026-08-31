#include <userver/storages/rocks/client.hpp>

#include <fmt/format.h>
#include <rocksdb/version.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/checkpoint.h>

#include <userver/engine/async.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/storages/rocks/cursor.hpp>
#include <userver/storages/rocks/exception.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

namespace {

#if ROCKSDB_MAJOR > 9
using RawDbPointerType = std::unique_ptr<rocksdb::DB> db_;
#else
using RawDbPointerType = rocksdb::DB*;
#endif

std::shared_ptr<rocksdb::DB> OpenPlain(const std::string& db_path) {
    rocksdb::Options options;
    options.create_if_missing = true;

    RawDbPointerType db_raw{};
    const rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_raw);
    if (!status.ok()) {
        throw RequestFailedException("Create client", status.ToString());
    }
    return std::shared_ptr<rocksdb::DB>(db_raw);
}

}  // namespace

Client::Client(const std::string& db_path, TransactionType txn_type)
    : blocking_task_processor_(engine::current_task::GetBlockingTaskProcessor())
{
    rocksdb::Options options;
    options.create_if_missing = true;

    if (txn_type == TransactionType::kPessimistic) {
        rocksdb::TransactionDB* raw{};
        const rocksdb::Status
            status = rocksdb::TransactionDB::Open(options, rocksdb::TransactionDBOptions{}, db_path, &raw);
        if (!status.ok()) {
            throw RequestFailedException("Create client (pessimistic)", status.ToString());
        }
        txn_db_ = raw;
        db_.reset(raw);
    } else if (txn_type == TransactionType::kOptimistic) {
        rocksdb::OptimisticTransactionDB* raw{};
        const rocksdb::Status status = rocksdb::OptimisticTransactionDB::Open(options, db_path, &raw);
        if (!status.ok()) {
            throw RequestFailedException("Create client (optimistic)", status.ToString());
        }
        opt_txn_db_ = raw;
        db_.reset(raw);
    } else {
        db_ = OpenPlain(db_path);
    }
}

void Client::Put(std::string_view key, std::string_view value) {
    engine::AsyncNoTracing(blocking_task_processor_, [this, key, value] {
        const rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
        CheckStatus(status, "Put");
    }).Get();
}

std::optional<std::string> Client::Get(std::string_view key) {
    return engine::AsyncNoTracing(
               blocking_task_processor_,
               [this, key]() -> std::optional<std::string> {
                   std::string res;
                   const rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &res);
                   if (status.IsNotFound()) {
                       return std::nullopt;
                   }
                   CheckStatus(status, "Get");
                   return res;
               }
    ).Get();
}

void Client::Delete(std::string_view key) {
    engine::AsyncNoTracing(blocking_task_processor_, [this, key] {
        const rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key);
        CheckStatus(status, "Delete");
    }).Get();
}

Snapshot Client::CreateSnapshot() {
    const rocksdb::Snapshot* snap = db_->GetSnapshot();
    if (snap == nullptr) {
        throw Exception("Failed to acquire snapshot");
    }
    return Snapshot(db_, blocking_task_processor_, snap);
}

Cursor Client::Scan(std::string_view prefix) {
    const rocksdb::Snapshot* snap = db_->GetSnapshot();
    if (snap == nullptr) {
        throw Exception("Failed to acquire snapshot for Scan");
    }
    return Cursor(db_, blocking_task_processor_, snap, std::string(prefix));
}

Transaction Client::BeginTransaction() {
    if (txn_db_ != nullptr) {
        auto* raw = txn_db_->BeginTransaction(rocksdb::WriteOptions());
        return Transaction(db_, blocking_task_processor_, std::unique_ptr<rocksdb::Transaction>(raw));
    }
    if (opt_txn_db_ != nullptr) {
        auto* raw = opt_txn_db_->BeginTransaction(rocksdb::WriteOptions());
        return Transaction(db_, blocking_task_processor_, std::unique_ptr<rocksdb::Transaction>(raw));
    }
    throw Exception("BeginTransaction: transactions not enabled; set transaction-type in config");
}

void Client::CheckStatus(rocksdb::Status status, std::string_view method_name) {
    if (!status.ok() && !status.IsNotFound()) {
        throw USERVER_NAMESPACE::storages::rocks::RequestFailedException(method_name, status.ToString());
    }
}

}  // namespace storages::rocks

USERVER_NAMESPACE_END
