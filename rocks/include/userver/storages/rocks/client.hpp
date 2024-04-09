#pragma once

/// @file userver/storages/rocks/client.hpp
/// @brief @copybrief storages::rocks::Client

#include <string>
#include <string_view>

#include <rocksdb/db.h>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

    /**
     * \brief Client for working with RocksDB storage.
     *
     * This class provides an interface for interacting with the RocksDB database.
     * To use the class, you need to specify the database path when creating an object.
     */
    class Client {
    public:
        /**
         * \brief Constructor of the Client class.
         *
         * \param db_path The path to the RocksDB database.
         */
        Client(const std::string& db_path);
        
        /**
         * \brief Puts a record into the database.
         *
         * \param key The key of the record.
         * \param value The value of the record.
         */
        void Put(std::string_view key, std::string_view value);

        /**
         * \brief Retrieves the value of a record from the database by key.
         *
         * \param key The key of the record.
         */
        std::string Get(std::string_view key);

        /**
         * Checks the status of an operation and handles any errors based on the given method name.
         * 
         * @param status The status of the operation to check.
         * @param method_name The name of the method associated with the operation.
         */
        void CheckStatus(rocksdb::Status status, std::string_view method_name);

        /**
         * \brief Deletes a record from the database by key.
         *
         * \param key The key of the record to be deleted.
         */
        void Delete(std::string_view key);

        virtual ~Client() {
            delete db_;
        }

    private:
        rocksdb::DB* db_ = nullptr;
    };
} // namespace storages::rocks

USERVER_NAMESPACE_END
