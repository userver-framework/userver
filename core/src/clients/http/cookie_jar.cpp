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
#include <userver/utils/text.hpp>

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

//  Some kind of validated/preprocessed cookie from original one
//  After preprocessing original cookie is not needed anymore, that's why ValidatedCookie contains only subset of attributes
struct ValidatedCookie {
    //  Original values
    std::string name;   
    std::string value;  
    bool secure;        
    //  Preprocessed attributes
    std::string domain = {}; 
    std::string path = {};
    bool host_only = false;
    bool prefix_secure = false;
    bool prefix_host = false;
    std::chrono::system_clock::time_point creation_time = {};
    std::chrono::system_clock::time_point expire_time = {};
};

static bool CookieTailMatch(std::string_view cookie_domain, std::string_view hostname) {
    if (hostname.length() < cookie_domain.length()) {
        return false;
    }

    auto hostname_suffix = hostname.substr(hostname.length() - cookie_domain.length());
    if (hostname_suffix != cookie_domain) {
        return false;
    }

   /*
   * A lead char of cookie_domain is not '.'.
   * RFC6265 4.1.2.3. The Domain Attribute says:
   * For example, if the value of the Domain attribute is
   * "example.com", the user agent will include the cookie in the Cookie
   * header when making HTTP requests to example.com, www.example.com, and
   * www.corp.example.com.
   */
    if (hostname.length() == cookie_domain.length()) {
        return true;
    }
    
    char char_before_suffix = hostname[hostname.length() - cookie_domain.length() - 1];
    return char_before_suffix == '.';
}

bool PathMatch(const ValidatedCookie& cookie, const std::string& uri_path) {
    
    //  Matching cookie path and URL path
    //  RFC6265 5.1.4 Paths and Path-Match
    //  Note: implementation is based partially on libcurl's source code

    /* cookie_path must not have last '/' separator. ex: /sample */
    if(cookie.path.size() == 1) {
        /* cookie_path must be '/' */
        return true;
    }

    std::string_view uri_view = uri_path;
    /* #-fragments are already cut off! */
    if(uri_view.empty() || uri_view[0] != '/')
        uri_view = "/";

    /*
    * here, RFC6265 5.1.4 says
    *  4. Output the characters of the uri-path from the first character up
    *     to, but not including, the right-most %x2F ("/").
    *  but URL path /hoge?fuga=xxx means /hoge/index.cgi?fuga=xxx in some site
    *  without redirect.
    *  Ignore this algorithm because /hoge is uri path for this case
    *  (uri path is not /).
    */
    if (uri_path.size() < cookie.path.size()) {
        return false;
    }

    /* not using checkprefix() because matching should be case-sensitive */
    
    if(cookie.path.starts_with(uri_view)) {
        return false;
    }

    /* The cookie-path and the uri-path are identical. */
    if(cookie.path.size() == uri_view.size()) {
        return true;
    }

    /* here, cookie_path_len < uri_path_len */
    if(uri_path[cookie.path.size()] == '/') {
        return true;
    }

    return false;
}

}  // namespace

class CookieJar::Impl {
public:
    void AddCookie(const std::string& domain, const std::string& path, server::http::Cookie&& cookie) {
        auto preprocessed_cookie = ValidateCookie(domain, path, cookie);
        if (!preprocessed_cookie.has_value()) {
            return;
        }
        if (preprocessed_cookie->expire_time <= utils::datetime::Now()) {
            DeleteCookie(*preprocessed_cookie);
            return;
        }
        auto location = storage_.try_emplace(preprocessed_cookie->domain, CookieNamesMap{});
        InsertOrAssignCookieToMap(location.first->second, *preprocessed_cookie);
        return;
    }

    std::optional<std::string> GetAnyCookieValue(const std::string& name) {
        for (const auto& item : storage_) {
            const auto& location = item.second.find(name);
            if (!location->second.empty()) {
                return location->second.front().value;
            }
        }
        return std::nullopt;
    }

    std::vector<std::pair<std::string, std::string>> GetCookies(const std::string& domain, const std::string& path) {
        std::vector<std::pair<std::string, std::string>> result;
        const auto& location = storage_.find(domain);
        if (location == storage_.end()) {
            return result;
        }
        for (const auto& cookies : location->second) {
            for (const auto& cookie : cookies.second) {
                // Temporary hack, think abou proper way to pass uri
                if (!PathMatch(cookie, domain + path)) {
                    continue;
                }
                if (cookie.host_only) {
                    if (domain != cookie.domain) {
                        continue;
                    }
                } else {
                    if (!CookieTailMatch(cookie.domain, domain)) {
                        continue;
                    }
                }
                result.emplace_back(cookie.name, cookie.value);
            }
        }
        return result;
    }

private:
// Vector optimized to store small count of elements
    template <typename Value>
    using SmallVectorStorage = boost::container::small_vector<Value, 3>;
// Cookies, which differs only in path property
    using CookiesList = SmallVectorStorage<ValidatedCookie>;
// Map from cookie name to list of cookies 
    using CookieNamesMap = std::unordered_map<std::string, CookiesList, utils::StrCaseHash>;
// Hashtable from domain to map of cookies
    using Storage = std::unordered_map<std::string, CookieNamesMap, utils::StrIcaseHash>;

    void DeleteCookie(const ValidatedCookie& cookie) {
        const auto location = storage_.find(cookie.domain);
        if (location == storage_.end()){
            return;
        }
        DeleteCookieFromMap(location->second, cookie);
    }

    static std::optional<ValidatedCookie> ValidateCookie(const std::string& domain, const std::string& path, server::http::Cookie& raw_cookie) {
        ValidatedCookie result{
            .name = raw_cookie.Name(), 
            .value = raw_cookie.Value(),
            .secure = raw_cookie.IsSecure()
        };
        {
            //  Preprocessing domain attribute
            std::string_view cookie_domain = domain;
            const bool host_only = raw_cookie.Domain().empty();
            if (!raw_cookie.Domain().empty()) {
                cookie_domain = raw_cookie.Domain();
            }
            if (!cookie_domain.empty() && cookie_domain[0] == '.') {
                // RFC 6265 5.2.3. Let cookie-domain be the attribute-value without the leading %x2E (".") character.
                cookie_domain = cookie_domain.substr(1);
            }
            if (cookie_domain.empty()) {
                // RFC 6265 5.2.3.If the attribute-value is empty, the behavior is undefined.  
                // However, the user agent SHOULD ignore the cookie-av entirely.
                LOG_WARNING() << "Ignoring cookie without domain attribute: '" << raw_cookie.Name() << "'";
                return std::nullopt;
            }
            auto lowered_domain = utils::text::ToLower(cookie_domain);
            if (IsPublicSuffix(lowered_domain)) {
                LOG_WARNING() << "Attempt to set supercookie: '" << raw_cookie.Name() << "' with domain '" << lowered_domain << "'";
                return std::nullopt;
            }
            if (!CookieTailMatch(lowered_domain, domain)) {
                LOG_WARNING() << "Attempt to set cookie with not matched domain: '" << raw_cookie.Name() << "' with domain '" << lowered_domain << "'";
                return std::nullopt;
            }
            result.host_only = host_only;
            result.domain = std::move(lowered_domain);
        }
        {
            // Preprocessing cookie path RFC 5.2.4
            std::string_view cookie_path = path;
            if (!raw_cookie.Path().empty() && raw_cookie.Path()[0] == '/') {
                cookie_path = raw_cookie.Path();
            }
            result.path = cookie_path;
        }
        {
            // Preprocessing time related attributes
            // RFC 6265 §5.3: a Set-Cookie is a deletion request when its Max-Age is <= 0
            // or, in the absence of Max-Age, its Expires lies in the past. Max-Age takes
            // precedence over Expires; a permanent cookie (Expires == time_point::max())
            // never expires.
            const auto& creation_time = utils::datetime::Now();
            auto expire_time = std::chrono::system_clock::time_point::max(); // By default it's without expiration time
            if (const auto max_age = raw_cookie.MaxAge()) {
                if (*max_age > std::chrono::seconds::zero()) {
                    expire_time =  creation_time + *max_age;
                } else {
                    expire_time = std::chrono::system_clock::time_point::min();
                }
            } else if (const auto expires = raw_cookie.Expires()) {
                expire_time = *expires;
            }
            result.creation_time = creation_time;
            result.expire_time = expire_time;
        }
        {
            //  Preprocessing prefixes
            if (result.name.starts_with("__Secure-")) {
                result.prefix_secure = true;
            } else if (result.name.starts_with("__Host-")) {
                result.prefix_host = true;
            }
            if (result.prefix_secure && !result.secure) {
                // The __Secure- prefix only requires that the cookie be set secure
                LOG_WARNING() << "Failed security check on cookie: '" << raw_cookie.Name() << "'";
                return std::nullopt;
            }
            if (result.prefix_host) {
                //The __Host- prefix requires the cookie to be secure, have a "/" path
                //and not have a domain set.
                if (!(result.secure && result.path == "/" && result.host_only)) {
                    LOG_WARNING() << "Failed host check on cookie: '" << raw_cookie.Name() << "'";
                    return std::nullopt;
                }
            }
        }
        return result;
    }

    static CookiesList::iterator FindDuplicate(CookiesList& list, const ValidatedCookie& cookie){
        return std::find_if(list.begin(), list.end(), 
            [&cookie](const ValidatedCookie& source_cookie) {
                return cookie.path == source_cookie.path;
            });
    }

    static void InsertOrAssignCookieToMap(CookieNamesMap& map, const ValidatedCookie& cookie) {
        auto location = map.try_emplace(cookie.name, CookiesList{});
        auto& list = location.first->second;
        auto cookie_location = FindDuplicate(list, cookie);
        if (cookie_location != list.end()) {
            *cookie_location = cookie;
            return;
            
        }
        list.push_back(cookie);
    }

    static void DeleteCookieFromMap(CookieNamesMap& map, const ValidatedCookie& cookie) {
        const auto list_location = map.find(cookie.name);
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

    Storage storage_;
};

CookieJar::CookieJar() = default;
CookieJar::~CookieJar() = default;

void CookieJar::AddCookie(const std::string& domain, const std::string& path, server::http::Cookie&& cookie) {
    impl_->AddCookie(domain, path, std::move(cookie));
}

std::optional<std::string> CookieJar::GetAnyCookieValue(const std::string& name) {
    return impl_->GetAnyCookieValue(name);
}

CookieJar::Cookies CookieJar::GetCookies(const std::string& domain, const std::string& path) {
    Cookies result;
    const auto domains = DomainCandidates(utils::text::ToLower(domain));

    for (const auto& d : domains) {
        auto domain_cookies = impl_->GetCookies(d, path);
        result.insert(result.end(), domain_cookies.begin(), domain_cookies.end());
    }

    return result;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
