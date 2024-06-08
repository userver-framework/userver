#include <userver/storages/rocks/client.hpp>

#include <fmt/format.h>

#include <userver/storages/rocks/exception.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

Client::Client(const std::string& db_path,
               engine::TaskProcessor& blocking_task_processor)
    : blocking_task_processor_(blocking_task_processor) {
  rocksdb::Options options;
  options.create_if_missing = true;

  rocksdb::DB* db{};
  rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
  db_.reset(db);
  CheckStatus(status, "Create client");
}

void Client::Put(std::string_view key, std::string_view value) {
  engine::AsyncNoSpan(blocking_task_processor_, [this, key, value] {
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
    CheckStatus(status, "Put");
  }).Get();
}

std::string Client::Get(std::string_view key) {
  return engine::AsyncNoSpan(blocking_task_processor_,
                             [this, key] {
                               std::string res;
                               rocksdb::Status status =
                                   db_->Get(rocksdb::ReadOptions(), key, &res);
                               CheckStatus(status, "Get");
                               return res;
                             })
      .Get();
}

void Client::Delete(std::string_view key) {
  return engine::AsyncNoSpan(blocking_task_processor_,
                             [this, key] {
                               rocksdb::Status status =
                                   db_->Delete(rocksdb::WriteOptions(), key);
                               CheckStatus(status, "Delete");
                             })
      .Get();
}

void Client::CheckStatus(rocksdb::Status status, std::string_view method_name) {
  if (!status.ok() && !status.IsNotFound()) {
    throw USERVER_NAMESPACE::storages::rocks::RequestFailedException(
        method_name, status.ToString());
  }
}
}  // namespace storages::rocks

USERVER_NAMESPACE_END
