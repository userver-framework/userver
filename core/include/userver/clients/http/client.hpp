#pragma once

/// @file userver/clients/http/client.hpp
/// @brief @copybrief clients::http::Client

#ifdef USERVER_TVM2_HTTP_CLIENT
#error Use clients::Http from clients/http.hpp instead
#endif

#include <userver/clients/http/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// @ingroup userver_clients
///
/// @brief HTTP client interface that returns a HTTP request builder from
/// CreateRequest().
///
/// Usually retrieved from @ref components::HttpClient component.
/// You can also create a client manually using @ref clients::http::CreateStandaloneHttpClient()
///
/// ## Example usage:
///
/// @snippet clients/http/client_test.cpp  Sample HTTP Client usage
class Client {
public:
    Client() = default;

    Client(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(const Client&) = delete;
    Client& operator=(Client&&) = delete;

    virtual ~Client();

    /// @brief Returns a HTTP request builder type with some preset values.
    ///
    /// @note This method is thread-safe despite being non-const.
    virtual Request CreateRequest() = 0;

    /// @brief Providing CreateNonSignedRequest() function for the clients::Http alias.
    ///
    /// @note This method is thread-safe despite being non-const.
    Request CreateNotSignedRequest() { return CreateRequest(); }
};

}  // namespace clients::http

USERVER_NAMESPACE_END
