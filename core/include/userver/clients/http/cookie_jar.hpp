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
    struct Impl;
public:
    class Cookie {
    public:
        Cookie(const std::string& name, const std::string& value, const std::chrono::system_clock::time_point& creation_time, const size_t path_length);
        
        inline const std::string& Name() const { 
            return name_; 
        }

        inline const std::string& Value() const { 
            return value_; 
        }

    private:
        friend class CookieJar::Impl;
        

        std::string name_;
        std::string value_;
        std::chrono::system_clock::time_point creation_time_;
        size_t path_length_;
    };

    using Cookies = std::vector<Cookie>;
    CookieJar();
    ~CookieJar();

    CookieJar(const CookieJar&) = delete;
    CookieJar(CookieJar&&) = delete;

    void AddCookie(const std::string& domain, const std::string& path, server::http::Cookie&& cookie);
    std::optional<std::string> GetAnyCookieValue(const std::string& name);
    Cookies GetCookies(const std::string& domain, const std::string& path);

private:
    utils::FastPimpl<Impl, 96, 8> impl_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
