#pragma once

/// @file userver/storages/sqlite/exceptions.hpp

#include <stdexcept>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief Base class for all uSQLite driver exceptions
class SQLiteException : public std::runtime_error {
public:
    SQLiteException(const char* error_message, int error_code, int extended_error_code);

    SQLiteException(const std::string& error_message, int error_code, int extended_error_code);

    SQLiteException(const char* error_message, int error_code);

    SQLiteException(const std::string& error_message, int error_code);

    explicit SQLiteException(const char* error_message);

    explicit SQLiteException(const std::string& error_message);

    ~SQLiteException() override;

    /// Return the result code (if any, otherwise -1).
    int getErrorCode() const noexcept;

    /// Return the extended numeric result code (if any, otherwise -1).
    int getExtendedErrorCode() const noexcept;

    /// Return a string, solely based on the error code
    const char* getErrorStr() const noexcept;

private:
    int error_code_;           // Error code value
    int extended_error_code_;  // Detailed error code if any
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
