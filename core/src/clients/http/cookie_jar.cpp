#include <userver/clients/http/cookie_jar.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <list>

#include <userver/utils/datetime.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

// Every stored cookie-domain that domain-matches the request host, per                                                                                                                           
// RFC 6265 §5.1.3 (Domain Matching): the host itself plus each of its parent                                                                                                                     
// domains. A stored cookie domain-matches iff it equals one of these (a cookie                                                                                                                   
// set for "example.com" is thus returned for "www.example.com" too).  
std::vector<std::string> DomainCandidates(std::string_view host) {
    std::vector<std::string> out;
    std::string_view cur = host;
    while (true) {
        out.emplace_back(cur);
        const auto dot = cur.find('.');
        if (dot == std::string_view::npos) break;
        cur = cur.substr(dot + 1);
    }
    return out;
}

//bool IsSecureScheme(std::string_view scheme) { return utils::StrIcaseEqual{}(scheme, "https"); }

// RFC 6265 §5.3: a Set-Cookie is a deletion request when its Max-Age is <= 0
// or, in the absence of Max-Age, its Expires lies in the past. Max-Age takes
// precedence over Expires; a permanent cookie (Expires == time_point::max())
// never expires.
bool IsExpiredCookie(const server::http::Cookie& cookie) {
    if (const auto max_age = cookie.MaxAge()) {
        return *max_age <= std::chrono::seconds::zero();
    }
    if (const auto expires = cookie.Expires()) {
        return *expires <= utils::datetime::Now();
    }
    return false;
}

}  // namespace

class CookieJar::Impl {
public:
    void AddCookie(const std::string& domain, const std::string& path, server::http::Cookie&& cookie) {
        if (!ValidateCookie(domain, path, cookie)) {
            LOG_WARNING() << "Could not validate cookie: '" << cookie.Name() << "' rejecting it";
            return;
        }
        if (IsExpiredCookie(cookie)) {
            DeleteCookie(cookie);
            return;
        }
        auto location = storage.try_emplace(cookie.Domain(), CookieNamesMap{});
        InsertOrAssignCookieToMap(location.first->second, cookie);
        return;
    }

    void DeleteCookie(const server::http::Cookie& cookie) {
        const auto location = storage.find(cookie.Domain());
        if (location == storage.end()){
            return;
        }
        DeleteCookieFromMap(location->second, cookie);
    }

    std::vector<server::http::Cookie> GetCookies(const std::string&, const std::string&) {
        return {};
    }

private:
// List of cookies, which differents only in path property
//  TODO: replace by intrusive list?
    using CookiesList = std::list<server::http::Cookie>;
// Map from cookie name to list of cookies 
    using CookieNamesMap = std::unordered_map<std::string, CookiesList, utils::StrCaseHash>;
// Hashtable from domain to map of cookies
    using Storage = std::unordered_map<std::string, CookieNamesMap, utils::StrIcaseHash>;

    static bool ValidateCookie(const std::string& domain, const std::string& path, server::http::Cookie& cookie) {
        if (cookie.Domain().empty()) {
            cookie.SetDomain(domain);
        }
        if (cookie.Path().empty()) {
            cookie.SetPath(path);
        }
        return true;
    }

    static CookiesList::iterator FindDuplicate(CookiesList& list, const server::http::Cookie& cookie){
        return std::find_if(list.begin(), list.end(), 
            [&cookie](const server::http::Cookie& source_cookie) {
                return cookie.Path() == source_cookie.Path();
            });
    }

    static void InsertOrAssignCookieToMap(CookieNamesMap& map, const server::http::Cookie& cookie) {
        auto location = map.try_emplace(cookie.Name(), CookiesList{});
        auto& list = location.first->second;
        auto cookie_location = FindDuplicate(list, cookie);
        if (cookie_location != list.end()) {
            *cookie_location = cookie;
            return;
            
        }
        list.push_back(cookie);
    }

    static void DeleteCookieFromMap(CookieNamesMap& map, const server::http::Cookie& cookie) {
        const auto list_location = map.find(cookie.Name());
        if (list_location == map.end()){
            return;
        }
        // Removing cookie from list with the same path
        auto& list = list_location->second;
        auto cookie_location = FindDuplicate(list, cookie);
        if (cookie_location != list.end()) {
            list.erase(cookie_location);
        }
        // Cleaning empty list
        if (list.empty()) {
            map.erase(list_location);
        }
    }

    Storage storage;
};

CookieJar::CookieJar() = default;
CookieJar::~CookieJar() = default;

void CookieJar::AddCookie(const std::string& domain, const std::string& path, Cookie&& cookie) {
    impl_->AddCookie(domain, path, Cookie{cookie});
}

CookieJar::Cookies CookieJar::GetCookies(const std::string& domain, const std::string& path) {
    Cookies result;
    const auto domains = DomainCandidates(domain);

    for (const auto& d : domains) {
        auto domain_cookies = impl_->GetCookies(d, path);
        result.insert(result.end(), domain_cookies.begin(), domain_cookies.end());
    }

    return result;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
