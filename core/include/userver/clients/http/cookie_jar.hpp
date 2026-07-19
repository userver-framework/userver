#pragma once

/// @file userver/clients/http/cookie_jar.hpp
/// @brief @copybrief clients::http::CookieJar

#include <cstddef>
#include <string>
#include <vector>

#include <userver/server/http/http_response_cookie.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// @brief Cookies storage
class CookieJar final {
public:
    using Cookie = std::pair<std::string, std::string>;
    using Cookies = std::vector<Cookie>;
    CookieJar();
    ~CookieJar();

    CookieJar(const CookieJar&) = delete;
    CookieJar(CookieJar&&) = delete;

    void AddCookie(const std::string& domain, const std::string& path, server::http::Cookie&& cookie);
    std::optional<std::string> GetAnyCookieValue(const std::string& name);
    Cookies GetCookies(const std::string& domain, const std::string& path);

private:
    struct Impl;
    utils::FastPimpl<Impl, 96, 8> impl_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
