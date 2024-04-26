#include <userver/storages/rocks/client.hpp>

#include <fmt/format.h>

#include <userver/storages/rocks/impl/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

Client::Client(const std::string& db_path) {
  rocksdb::Options options;
  options.create_if_missing = true;
  // TODO: Do in another task processor, in the current implementation blocking
  // wait in the current coroutine
  rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_);
  CheckStatus(status, "Create client");
}

void Client::Put(std::string_view key, std::string_view value) {
  // TODO: Do in another task processor, in the current implementation blocking
  // wait in the current coroutine
  rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
  CheckStatus(status, "Put");
}

std::string Client::Get(std::string_view key) {
  std::string res;
  // TODO: Do in another task processor, in the current implementation blocking
  // wait in the current coroutine
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &res);
  CheckStatus(status, "Get");
  return res;
}

void Client::Delete(std::string_view key) {
  // TODO: Do in another task processor, in the current implementation blocking
  // wait in the current coroutine
  rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key);
  CheckStatus(status, "Delete");
}

void Client::CheckStatus(rocksdb::Status status, std::string_view method_name) {
  if (!status.ok() && !status.IsNotFound()) {
    throw USERVER_NAMESPACE::storages::rocks::impl::RequestFailedException(
        method_name, status.ToString());
  }
}
}  // namespace storages::rocks

USERVER_NAMESPACE_END
