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
    CookieJar() = default;

    /// @brief Gets ANY cookie value, associated with name. In general case, multiple cookies can be stored with the same name, order is not specified
    /// @param name Name of cookie
    /// @return Cookie's value
    /// @warning This method has linear complexity
    std::optional<std::string> FindCookieValue(std::string_view name) const;
private:
    // Constructs CookieJar by list of cookies in netscape file format
    explicit CookieJar(std::vector<std::string>&& cookies);

    // To allow request to construct/extract cookies in netscape format
    friend class Request;

    //  List of cookies in netscape file format, directly exposed for libcurl engine
    //  Other format like Set-Cookie can have side effects, for more information see libcurl docs
    std::vector<std::string> cookies_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
