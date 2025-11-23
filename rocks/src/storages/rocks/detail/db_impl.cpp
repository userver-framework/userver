#include <userver/storages/rocks/detail/db_impl.hpp>

#include <stdexcept>

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/iterator.h>

#include <userver/utils/async.hpp>
#include <userver/logging/log.hpp>

#include <userver/storages/rocks/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks::detail {

DbImpl::DbImpl(const rocksdb::Options& options, const std::string& db,
        const std::vector<std::string>& column_families, engine::TaskProcessor& task_processor)
    : task_processor_{task_processor}, column_family_handles_{} {
    // Prepare column family descriptors (they are useless after opening the database).
    std::vector<rocksdb::ColumnFamilyDescriptor> descriptors{};
    for (auto&& name : column_families) {
        descriptors.push_back(
            rocksdb::ColumnFamilyDescriptor{name, rocksdb::ColumnFamilyOptions{}}
        );
    }
    rocksdb::DB* dbptr{};
    std::vector<rocksdb::ColumnFamilyHandle*> handles{};
    auto status = rocksdb::DB::Open(options, db, descriptors, &handles, &dbptr);
    CheckStatus(status, "Open");
    // Transfer descriptors from a simple vector to a hash table.
    for (auto&& handle : handles) {
        column_family_handles_[handle->GetName()] = handle;
    }
    // Complete the initialization by setting the smart pointer with current database instance.
    db_.reset(dbptr);
}

// https://github.com/facebook/rocksdb/wiki/Basic-Operations#closing-a-database
DbImpl::~DbImpl() {
    auto task =
        engine::AsyncNoSpan(task_processor_, [this]() {
            rocksdb::Status status;
            if (status = db_->SyncWAL(); !status.ok()) {
                LOG_ERROR() << "Error synchronizing WAL: " << status.ToString();
            }
            for (const auto& [name, handle] : this->column_family_handles_) {
                if (status = db_->DestroyColumnFamilyHandle(handle); !status.ok()) {
                    LOG_ERROR() << "Error destroying column family handle (" << name << "): " << status.ToString();
                }
            }
            if (status = db_->Close(); !status.ok()) {
                LOG_ERROR() << "Error closing: " << status.ToString();
            }
        });
    if (task.WaitNothrow() == false) {
        LOG_ERROR() << "Failed to close RocksDB gracefully";
    }
}

rocksdb::ColumnFamilyHandle* DbImpl::GetColumnFamily(const std::string& name) const {
    if (auto it = column_family_handles_.find(name); it != column_family_handles_.end()) {
        return it->second;
    }
    throw std::runtime_error("No such column family is configured");
}

void DbImpl::Put(const rocksdb::WriteOptions& options, std::string_view key, std::string_view value) {
    auto task = engine::AsyncNoSpan(task_processor_, [&]() { return db_->Put(options, key, value); });
    CheckStatus(task.Get(), "Put");
}

void DbImpl::Put(const rocksdb::WriteOptions& options, rocksdb::ColumnFamilyHandle* column_family,
        std::string_view key, std::string_view value) {
    auto task = engine::AsyncNoSpan(task_processor_, [&]() { return db_->Put(options, column_family, key, value); });
    CheckStatus(task.Get(), "Put");
}

void DbImpl::Delete(const rocksdb::WriteOptions& options, std::string_view key) {
    auto task = engine::AsyncNoSpan(task_processor_, [&]() { return db_->Delete(options, key); });
    CheckStatus(task.Get(), "Delete");
}

void DbImpl::Delete(const rocksdb::WriteOptions& options, rocksdb::ColumnFamilyHandle* column_family,
        std::string_view key) {
    auto task = engine::AsyncNoSpan(task_processor_, [&]() { return db_->Delete(options, column_family, key); });
    CheckStatus(task.Get(), "Delete");
}

void DbImpl::Write(const rocksdb::WriteOptions& options, rocksdb::WriteBatch& write_batch) {
    auto task = engine::AsyncNoSpan(task_processor_, [&]() { return db_->Write(options, &write_batch); });
    CheckStatus(task.Get(), "Write");
}

std::optional<std::string> DbImpl::Get(const rocksdb::ReadOptions& options, std::string_view key) const {
    std::string value;
    auto status = engine::AsyncNoSpan(task_processor_, [&]() { return db_->Get(options, key, &value); }).Get();
    if (status.IsNotFound()) return {};
    CheckStatus(status, "Get");
    return value;
}

std::optional<std::string> DbImpl::Get(const rocksdb::ReadOptions& options, rocksdb::ColumnFamilyHandle* column_family,
        std::string_view key) const {
    std::string value;
    auto status = engine::AsyncNoSpan(task_processor_,
        [&]() { return db_->Get(options, column_family, key, &value); }).Get();
    if (status.IsNotFound()) return {};
    CheckStatus(status, "Get");
    return value;
}

std::unique_ptr<rocksdb::Iterator> DbImpl::NewIterator(const rocksdb::ReadOptions& options) const {
    return std::unique_ptr<rocksdb::Iterator>(db_->NewIterator(options));
}

std::unique_ptr<rocksdb::Iterator> DbImpl::NewIterator(const rocksdb::ReadOptions& options,
        rocksdb::ColumnFamilyHandle* column_family) const {
    return std::unique_ptr<rocksdb::Iterator>(db_->NewIterator(options, column_family));
}

const rocksdb::Snapshot* DbImpl::GetSnapshot() const {
    return db_->GetSnapshot();
}

void DbImpl::ReleaseSnapshot(const rocksdb::Snapshot* snapshot) const {
    db_->ReleaseSnapshot(snapshot);
}

engine::TaskProcessor& DbImpl::GetTaskProcessor() {
    return task_processor_;
}

}  // namespace storages::rocks::detail

USERVER_NAMESPACE_END
