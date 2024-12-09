#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <userver/clients/http/error_kind.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

/// Base exception for all the client exceptions
class Exception : public std::exception {
    virtual std::string_view HandleInfo() const noexcept = 0;
};

/// Base exception class for response with code
///
/// @warning your exception type should also derive from one of the
/// std::exception based exceptions.
class ExceptionWithStatusCode
/* intentionally not inherited from std::exception */ {
public:
    explicit ExceptionWithStatusCode(int status_code);

    virtual ~ExceptionWithStatusCode();

    int GetStatusCode() const;

    void SetHeaders(std::unordered_map<std::string, std::string>&& headers);

    std::string GetHeader(const std::string& name) const;

private:
    int status_code_;
    std::unordered_map<std::string, std::string> headers_;
};

/// Base exception class for response with transport error kind
///
/// @warning your exception type should also derive from one of the
/// std::exception based exceptions.
class HttpException /* intentionally not inherited from std::exception */
{
public:
    explicit HttpException(clients::http::ErrorKind error_kind);

    virtual ~HttpException();

    clients::http::ErrorKind GetErrorKind() const;

private:
    clients::http::ErrorKind error_kind_;
};

/// @brief Additional base exception type for all the timeout related client
/// exceptions.
///
/// @warning your exception type should also derive from one of the
/// std::exception based exceptions.
class TimeoutException /* intentionally not inherited from std::exception */ {
public:
    virtual ~TimeoutException();
};

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
