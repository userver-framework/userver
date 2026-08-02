#include <userver/clients/http/cookie_jar.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

CookieJar::CookieJar() = default;
CookieJar::~CookieJar() = default;
CookieJar::CookieJar(const CookieJar&) = default;
CookieJar::CookieJar(CookieJar&&) = default;
CookieJar& CookieJar::operator=(const CookieJar&) = default;
CookieJar& CookieJar::operator=(CookieJar&&) = default;

CookieJar::CookieJar(std::vector<std::string>&& cookies) : 
    cookies_(std::move(cookies)) {
}

std::optional<std::string> CookieJar::FindCookieValue(std::string_view name) const {
    //  TODO: not optimal way to search
    for (std::string_view line : cookies_) {
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.remove_suffix(1);
        }

        const auto value_pos = line.rfind('\t');
        if (value_pos == std::string_view::npos || value_pos == 0) {
            // A comment or a malformed line, both have nothing to look at.
            continue;
        }
        const auto name_pos = line.rfind('\t', value_pos - 1);
        if (name_pos == std::string_view::npos) {
            continue;
        }

        // Cookie names are case-sensitive, RFC 6265, section 5.3.
        if (line.substr(name_pos + 1, value_pos - name_pos - 1) == name) {
            return std::string{line.substr(value_pos + 1)};
        }
    }
    return std::nullopt;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
