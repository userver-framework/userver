#pragma once

/// @file userver/storages/rocks/client.hpp
/// @brief @copybrief storages::rocks::Client

#include <string>
#include <string_view>

#include <rocksdb/db.h>

#include <userver/engine/task/task_processor_fwd.hpp>

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
   * @param blocking_task_processor - task processor to execute blocking FS
   * operations
   */
  Client(const std::string& db_path,
         engine::TaskProcessor& blocking_task_processor);

  /**
   * @brief Puts a record into the database.
   *
   * @param key The key of the record.
   * @param value The value of the record.
   */
  void Put(std::string_view key, std::string_view value);

  /**
   * @brief Retrieves the value of a record from the database by key.
   *
   * @param key The key of the record.
   */
  std::string Get(std::string_view key);

  /**
   * @brief Deletes a record from the database by key.
   *
   * @param key The key of the record to be deleted.
   */
  void Delete(std::string_view key);

  /**
   * Checks the status of an operation and handles any errors based on the given
   * method name.
   *
   * @param status The status of the operation to check.
   * @param method_name The name of the method associated with the operation.
   */
  void CheckStatus(rocksdb::Status status, std::string_view method_name);

 private:
  std::unique_ptr<rocksdb::DB> db_;
  engine::TaskProcessor& blocking_task_processor_;
};
}  // namespace storages::rocks

USERVER_NAMESPACE_END
