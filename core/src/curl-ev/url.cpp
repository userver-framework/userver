#include <curl-ev/url.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

bool HasUrlUserinfo(std::string_view url) noexcept {
    const auto url_start = url.find_first_not_of(" \t\r\n\v\f");
    if (url_start == std::string_view::npos) {
        return false;
    }
    url.remove_prefix(url_start);

    if (!url.starts_with("//")) {
        const auto scheme_end = url.find_first_of(":/?#");
        if (scheme_end == std::string_view::npos || url[scheme_end] != ':') {
            return false;
        }
        url.remove_prefix(scheme_end + 1);
        if (!url.starts_with('/')) {
            return false;
        }
    }

    const auto authority_start = url.find_first_not_of('/');
    if (authority_start == std::string_view::npos) {
        return false;
    }
    url.remove_prefix(authority_start);
    const auto authority = url.substr(0, url.find_first_of("/?#"));
    return authority.find('@') != std::string_view::npos;
}

url::url()
    : url_(native::curl_url())
{}

url::url(const url& other)
    : url_(other.url_ ? native::curl_url_dup(other.url_.get()) : nullptr)
{}

url& url::operator=(const url& rhs) {
    if (this == &rhs) {
        return *this;
    }

    *this = url{rhs};
    return *this;
}

void url::SetAbsoluteUrl(const char* url, std::error_code& ec, bool allow_userinfo) {
    SetHost(nullptr, ec);
    UASSERT(!ec);
    if (!ec && !allow_userinfo && HasUrlUserinfo(url)) {
        ec = errc::UrlErrorCode::kUserNotAllowed;
    }
    if (!ec) {
        const auto flags = CURLU_PATH_AS_IS | (allow_userinfo ? 0U : CURLU_DISALLOW_USER);
        ec = static_cast<errc::UrlErrorCode>(native::curl_url_set(url_.get(), native::CURLUPART_URL, url, flags));
    }
}

void url::SetAbsoluteUrl(const char* url) {
    std::error_code ec;
    SetAbsoluteUrl(url, ec);
    throw_error(ec, "SetAbsoluteUrl");
}

void url::SetDefaultSchemeUrl(const char* url, std::error_code& ec) {
    SetHost(nullptr, ec);
    UASSERT(!ec);
    if (!ec) {
        ec = std::error_code{static_cast<errc::UrlErrorCode>(
            native::curl_url_set(url_.get(), native::CURLUPART_URL, url, CURLU_DEFAULT_SCHEME | CURLU_PATH_AS_IS)
        )};
    }
}

}  // namespace curl

USERVER_NAMESPACE_END
