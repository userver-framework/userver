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

/// @brief Storage for cookies, compliable with RFC 6265. Can be used for sending and receiving cookies on agent side.
class CookieJar final {
    struct Impl;
public:
    /// @brief Extracted cookie from storage, holds auxiliary internal values like creation time/path length
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
        friend class CookieJar;

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

    /// @brief Merges cookie jar into current one. Can be useful for merging cookies from other requests/domains
    /// @param cookie_jar Cookie jar to merge
    void Merge(CookieJar&& cookie_jar);

    /// @brief Adds cookie with associated url to storage. In general case, url is needed to compute missing fields
    /// @param url Request URI cookie came from
    /// @param cookie Cookie to store
    //  TODO: not effective due url parsing, but simple api to use, optimize?
    void AddCookie(std::string_view url, server::http::Cookie&& cookie);

    /// @brief Gets ANY cookie value, associated with name. In general case, multiple cookies can be stored with the same name, order is not specified
    /// @param name Name of cookie
    /// @return Cookie's value
    std::optional<std::string> GetAnyCookieValue(const std::string& name);


    /// @brief Gets cookies, associated with current url. In general case, domain/path properties is taken into account
    /// @param url Url to be matched with cookies
    /// @return Ordered list of cookies
    Cookies GetCookies(std::string_view url);

private:
    utils::FastPimpl<Impl, 96, 8> impl_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
