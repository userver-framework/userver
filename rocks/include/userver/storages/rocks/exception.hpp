#pragma once

/// @file userver/storages/rocks/impl/exception.hpp
/// @brief rocks-specific exceptions

#include <stdexcept>
#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/// Generic rocks-related exception
class Exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// Request execution failed
class RequestFailedException : public Exception {
public:
    RequestFailedException(std::string_view request_description, std::string_view status);

    std::string_view GetStatusString() const;

private:
    std::string status_;
};

/// Thrown by Transaction::Commit() when an optimistic conflict is detected.
/// The transaction must be rolled back; the operation should be retried.
class WriteConflictException : public Exception {
public:
    using Exception::Exception;
};

/// Thrown by Transaction::GetForUpdate() when a pessimistic lock times out.
class LockTimeoutException : public Exception {
public:
    using Exception::Exception;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
