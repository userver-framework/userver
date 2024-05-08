#pragma once

/// @file userver/storages/rocks/client.hpp
/// @brief @copybrief storages::rocks::Client

#include <string>
#include <string_view>

#include <rocksdb/db.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/**
 * @brief Client for working with RocksDB storage.
 *
 * This class provides an interface for interacting with the RocksDB database.
 * To use the class, you need to specify the database path when creating an
 * object.
 */
class Client final {
 public:
  /**
   * @brief Constructor of the Client class.
   *
   * @param db_path The path to the RocksDB database.
   * @param task_processor is a thread pool on which the tasks (engine::Task, engine::TaskWithresult) are executed.
   */
  explicit Client(const std::string& db_path, engine::TaskProcessor& task_processor);

  /**
   * @brief Puts a record into the database.
   *
   * @param key The key of the record.
   * @param value The value of the record.
   */
  engine::TaskWithResult<void> Put(std::string_view key, std::string_view value);

  /**
   * @brief Retrieves the value of a record from the database by key.
   *
   * @param key The key of the record.
   */
  engine::TaskWithResult<std::string> Get(std::string_view key);

  /**
   * @brief Deletes a record from the database by key.
   *
   * @param key The key of the record to be deleted.
   */
  engine::TaskWithResult<void> Delete(std::string_view key);

  /**
   * Checks the status of an operation and handles any errors based on the given
   * method name.
   *
   * @param status The status of the operation to check.
   * @param method_name The name of the method associated with the operation.
   */
  void CheckStatus(rocksdb::Status status, std::string_view method_name);

  virtual ~Client() { delete db_; }

 private:
  rocksdb::DB* db_ = nullptr;
  engine::TaskProcessor& task_processor_;
};
}  // namespace storages::rocks

USERVER_NAMESPACE_END
