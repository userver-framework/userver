#include <userver/clients/http/cookie_jar.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <set>
#include <utility>
#include <vector>
#include <boost/container/small_vector.hpp>

#include <userver/utils/datetime.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

// TODO: not good, for full coverage see libpsl (used by curl), or something else
static const std::set<std::string> kPublicSuffixes = {
    // simple TLD
    "com", "org", "net", "edu", "gov", "mil", "int",
    
    //  TLD
    "us", "uk", "de", "jp", "cn", "ru", "br", "au", "ca", "fr",
    "it", "nl", "eu", "ch", "se", "pl", "in", "kr", "za", "mx",
    
    // eTLD
    "co.uk", "org.uk", "ac.uk", "gov.uk", 
    "com.au", "net.au", "org.au", "edu.au", "gov.au",
    "co.jp", "ne.jp", "or.jp", "ac.jp", "go.jp",
    "co.in", "net.in", "org.in", "ac.in", "res.in",
    "com.br", "net.br", "org.br", "edu.br", "gov.br",
    "com.ru", "net.ru", "org.ru", "pp.ru", 
    
    "appspot.com", "blogspot.com", "github.io", "githubpages.com"
};

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

bool IsPublicSuffix(const std::string& domain) {
    return kPublicSuffixes.find(domain) != kPublicSuffixes.end();
}

//bool IsSecureScheme(std::string_view scheme) { return utils::StrIcaseEqual{}(scheme, "https"); }

struct CookieInfo {
    server::http::Cookie cookie;
    std::chrono::system_clock::time_point creation_time;

    
    bool IsExpired() const {
        // RFC 6265 §5.3: a Set-Cookie is a deletion request when its Max-Age is <= 0
        // or, in the absence of Max-Age, its Expires lies in the past. Max-Age takes
        // precedence over Expires; a permanent cookie (Expires == time_point::max())
        // never expires.
        const auto& now = utils::datetime::Now();
        if (const auto max_age = cookie.MaxAge()) {
            if (*max_age > std::chrono::seconds::zero()) {
                return creation_time + *max_age <= now;
            }
            return true;
        }
        if (const auto expires = cookie.Expires()) {
            return *expires <= now;
        }
        return false;
    }
};

}  // namespace

class CookieJar::Impl {
public:
    void AddCookie(const std::string& domain, const std::string& path, server::http::Cookie&& cookie) {
        auto preprocessed_cookie = PreprocessCookie(domain, path, cookie);
        if (!preprocessed_cookie.has_value()) {
            return;
        }
        if (preprocessed_cookie->IsExpired()) {
            DeleteCookie(*preprocessed_cookie);
            return;
        }
        auto location = storage.try_emplace(cookie.Domain(), CookieNamesMap{});
        InsertOrAssignCookieToMap(location.first->second, *preprocessed_cookie);
        return;
    }

    void DeleteCookie(const CookieInfo& cookie) {
        const auto location = storage.find(cookie.cookie.Domain());
        if (location == storage.end()){
            return;
        }
        DeleteCookieFromMap(location->second, cookie);
    }

    std::vector<server::http::Cookie> GetCookies(const std::string&, const std::string&) {
        return {};
    }

private:
// Vector optimized to store small count of elements
    template <typename Value>
    using SmallVectorStorage = boost::container::small_vector<Value, 3>;
// Cookies, which differs only in path property
    using CookiesList = SmallVectorStorage<CookieInfo>;
// Map from cookie name to list of cookies 
    using CookieNamesMap = std::unordered_map<std::string, CookiesList, utils::StrCaseHash>;
// Hashtable from domain to map of cookies
    using Storage = std::unordered_map<std::string, CookieNamesMap, utils::StrIcaseHash>;

    static std::optional<CookieInfo> PreprocessCookie(const std::string& domain, const std::string& path, server::http::Cookie& cookie) {
        if (cookie.Domain().empty()) {
            cookie.SetDomain(domain);
        }
        if (cookie.Path().empty()) {
            cookie.SetPath(path);
        }
        if (IsPublicSuffix(domain)) {
            LOG_WARNING() << "Attempt to set supercookie: '" << cookie.Name() << "' with domain '" << domain << "'";
            return std::nullopt;
        }
        CookieInfo cookie_info{.cookie = cookie, .creation_time = utils::datetime::Now()};
        return cookie_info;
    }

    static CookiesList::iterator FindDuplicate(CookiesList& list, const CookieInfo& cookie_info){
        return std::find_if(list.begin(), list.end(), 
            [&cookie_info](const CookieInfo& source_cookie) {
                return cookie_info.cookie.Path() == source_cookie.cookie.Path();
            });
    }

    static void InsertOrAssignCookieToMap(CookieNamesMap& map, const CookieInfo& cookie_info) {
        auto location = map.try_emplace(cookie_info.cookie.Name(), CookiesList{});
        auto& list = location.first->second;
        auto cookie_location = FindDuplicate(list, cookie_info);
        if (cookie_location != list.end()) {
            *cookie_location = cookie_info;
            return;
            
        }
        list.push_back(cookie_info);
    }

    static void DeleteCookieFromMap(CookieNamesMap& map, const CookieInfo& cookie) {
        const auto list_location = map.find(cookie.cookie.Name());
        if (list_location == map.end()){
            return;
        }
        // Removing cookie from list with the same path
        auto& list = list_location->second;
        auto cookie_location = FindDuplicate(list, cookie);
        if (cookie_location != list.end()) {
            std::iter_swap(cookie_location, list.end() - 1);
            list.pop_back();
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
