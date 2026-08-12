#pragma once

/// @file userver/server/http/http_response_cookie.hpp
/// @brief @copybrief server::http::Cookie

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <userver/http/predefined_header.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @ingroup userver_containers
///
/// @brief HTTP response cookie
class Cookie final {
public:
    using CookiesMap = std::unordered_map<std::string, Cookie, utils::StrCaseHash>;

    static std::optional<Cookie> FromString(std::string_view cookie_str);

    Cookie(std::string name, std::string value);
    Cookie(Cookie&& cookie) noexcept;
    Cookie(const Cookie& cookie);
    ~Cookie() noexcept;

    Cookie& operator=(Cookie&&) noexcept;
    Cookie& operator=(const Cookie& cookie);

    const std::string& Name() const noexcept;
    const std::string& Value() const noexcept;

    bool IsSecure() const noexcept;
    Cookie& SetSecure() noexcept;

    std::chrono::system_clock::time_point Expires() const noexcept;
    Cookie& SetExpires(std::chrono::system_clock::time_point value) noexcept;

    bool IsPermanent() const noexcept;
    Cookie& SetPermanent();

    bool IsHttpOnly() const noexcept;
    Cookie& SetHttpOnly() noexcept;

    const std::string& Path() const noexcept;
    Cookie& SetPath(std::string value);

    const std::string& Domain() const noexcept;
    Cookie& SetDomain(std::string value);

    std::chrono::seconds MaxAge() const noexcept;
    Cookie& SetMaxAge(std::chrono::seconds value) noexcept;

    std::string SameSite() const;
    Cookie& SetSameSite(std::string value);

    std::string ToString() const;

    void AppendToString(USERVER_NAMESPACE::http::headers::HeadersString& os) const;

private:
    class CookieData;
    std::unique_ptr<CookieData> data_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
