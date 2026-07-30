#include <userver/clients/http/cookie_jar.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <set>
#include <utility>
#include <vector>
#include <algorithm>
#include <boost/container/small_vector.hpp>

#include <userver/utils/datetime.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/logging/log.hpp>
#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

CookieJar::CookieJar() = default;
CookieJar::~CookieJar() = default;
CookieJar::CookieJar(std::vector<std::string>&& cookies) : 
    cookies_(std::move(cookies)) {

}
CookieJar::CookieJar(const CookieJar&) = default;
CookieJar::CookieJar(CookieJar&&) = default;
CookieJar& CookieJar::operator=(const CookieJar&) = default;
CookieJar& CookieJar::operator=(CookieJar&&) = default;

std::optional<std::string> CookieJar::FindCookieValue(std::string_view name) {
    return std::nullopt;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
