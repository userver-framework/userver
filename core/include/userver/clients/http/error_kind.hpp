#pragma once

/// @file userver/clients/http/error_kind.hpp
/// @brief @copybrief clients::http::ErrorKind

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// Additional tag to the exception.
enum class ErrorKind {
    kNetwork,              // error during transportation
    kDeadlinePropagation,  // our own deadline exceeded
    kTimeout,              // timeout reached
    kCancel,               // task or request cancelled
    kClient,               // request was called with a bad parameter
    kServer,               // error on server side
};

}  // namespace clients::http

USERVER_NAMESPACE_END
