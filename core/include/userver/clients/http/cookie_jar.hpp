#pragma once

/// @file userver/clients/http/cookie_jar.hpp
/// @brief @copybrief clients::http::CookieJar
#include <string>
#include <vector>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class Request;

/// @brief Storage for cookies, compliable with RFC 6265. Can be used for sending and receiving cookies on agent side.
class CookieJar final {
public:
    CookieJar();
    CookieJar(std::vector<std::string>&& cookies);
    ~CookieJar();
    CookieJar(const CookieJar&);
    CookieJar(CookieJar&&);
    CookieJar& operator=(const CookieJar&);
    CookieJar& operator=(CookieJar&&);

    /// @brief Gets ANY cookie value, associated with name. In general case, multiple cookies can be stored with the same name, order is not specified
    /// @param name Name of cookie
    /// @return Cookie's value
    /// @warning This method has lineral complexity
    std::optional<std::string> FindCookieValue(std::string_view name);
private:
    friend class Request;
    std::vector<std::string> cookies_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
