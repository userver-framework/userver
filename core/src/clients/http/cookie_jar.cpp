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

namespace {

// TODO: not good, for full coverage see libpsl (used by curl), or something else
static const std::set<std::string, std::less<>> kPublicSuffixes = {
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

//  There is issue with calling userver's version from non coroutines. We temporary using this one
std::string ToLower(std::string_view str) {
    std::string result;
    result.resize(str.size());
    std::ranges::transform(str, result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool IsPublicSuffix(std::string_view domain) {
    return kPublicSuffixes.find(domain) != kPublicSuffixes.end();
}

bool IsSecureScheme(std::string_view scheme) { 
    return utils::StrIcaseEqual{}(scheme, "https"); 
}

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

bool PathMatch(const ValidatedCookie& cookie, std::string_view url_path) {
    if (cookie.path.size() <= 1) {
        return !cookie.path.empty();
    }

    if (url_path.empty() || url_path[0] != '/') {
        url_path = "/";
    }

    // Make sure the cookie path is a prefix of the url path.
    if (!url_path.starts_with(cookie.path)) return false;

    // In order to avoid in correctly matching a cookie path of /blah
    // with a request path of '/blahblah/', we need to make sure that either
    // the cookie path ends in a trailing '/', or that we prefix up to a '/'
    // in the url path.
    if (cookie.path.length() != url_path.length() && cookie.path.back() != '/' && url_path[cookie.path.length()] != '/') {
        return false;
    }
    return true;
}

static std::string LowerDomainWithoutLeadingDot(std::string_view domain) {
    if (!domain.empty() && domain[0] == '.') {
        // RFC 6265 5.2.3. Let cookie-domain be the attribute-value without the leading %x2E (".") character.
        domain = domain.substr(1);
    }
    return ToLower(domain);
}

}  // namespace

CookieJar::Cookie::Cookie(std::string_view name, std::string_view value, 
    const std::chrono::system_clock::time_point& creation_time, const size_t path_length) : 
    name_(name), value_(value), creation_time_(creation_time), path_length_(path_length){
}

class CookieJar::Impl {
public:

    void Merge(Impl& other) { 
        //  TODO: not so optimal way to merge in the case of many paths
        for (auto& item : other.storage_) {
            auto location = storage_.find(item.first);
            if (location == storage_.end()) {
                storage_.emplace(location->first, std::move(location->second));
                continue;
            }
            for (auto& cookie_map : location->second) {
                for (auto& validated_cookie : cookie_map.second) {
                    InsertOrAssignCookieToMap(std::move(validated_cookie));
                }
            }
        }
    }
    void AddCookie(std::string_view url, server::http::Cookie&& cookie) {
        const auto& parsed_url = userver::http::DecomposeUrlIntoViews(url);
        auto preprocessed_cookie = ValidateCookie(parsed_url.scheme, parsed_url.host, parsed_url.path, cookie);
        if (!preprocessed_cookie.has_value()) {
            return;
        }
        if (preprocessed_cookie->expire_time <= utils::datetime::Now()) {
            DeleteCookie(*preprocessed_cookie);
            return;
        }
        InsertOrAssignCookieToMap(std::move(preprocessed_cookie).value());
        return;
    }

    std::optional<std::string> GetAnyCookieValue(std::string_view name) {
        for (const auto& item : storage_) {
            const auto& location = item.second.find(name);
            if (!location->second.empty()) {
                return location->second.front().value;
            }
        }
        return std::nullopt;
    }

    void GetCookies(CookieJar::Cookies& out, std::string_view scheme, std::string_view domain, std::string_view path) {
        const auto& location = storage_.find(domain);
        if (location == storage_.end()) {
            return;
        }
        const bool secure_scheme = IsSecureScheme(scheme);
        for (const auto& cookies : location->second) {
            for (const auto& cookie : cookies.second) {
                if (cookie.secure && !secure_scheme) {
                    continue;
                }
                if (!PathMatch(cookie, path)) {
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
                out.emplace_back(cookie.name, cookie.value, cookie.creation_time, cookie.path.size());
            }
        }
    }

private:
// Vector optimized to store small count of elements
    template <typename Value>
    using SmallVectorStorage = boost::container::small_vector<Value, 3>;
// Cookies, which differs only in path property
    using CookiesList = SmallVectorStorage<ValidatedCookie>;
// Map from cookie name to list of cookies 
    using CookieNamesMap = std::unordered_map<std::string, CookiesList, utils::StrCaseHash, std::equal_to<>>;
// Hashtable from domain to map of cookies
    using Storage = std::unordered_map<std::string, CookieNamesMap, utils::StrIcaseHash, std::equal_to<>>;

    void DeleteCookie(const ValidatedCookie& cookie) {
        const auto location = storage_.find(cookie.domain);
        if (location == storage_.end()){
            return;
        }
        DeleteCookieFromMap(location->second, cookie);
    }

    static std::optional<ValidatedCookie> ValidateCookie(std::string_view scheme, std::string_view domain, 
            std::string_view path, server::http::Cookie& raw_cookie) {
        ValidatedCookie result{
            .name = raw_cookie.Name(), 
            .value = raw_cookie.Value(),
            .secure = raw_cookie.IsSecure()
        };
        {
            //  Preprocessing domain attribute
            const bool host_only = raw_cookie.Domain().empty();
            auto lowered_domain = LowerDomainWithoutLeadingDot(host_only ? domain : raw_cookie.Domain());
            if (lowered_domain.empty()) {
                // RFC 6265 5.2.3.If the attribute-value is empty, the behavior is undefined.  
                // However, the user agent SHOULD ignore the cookie-av entirely.
                LOG_WARNING() << "Ignoring cookie without domain attribute: '" << raw_cookie.Name() << "'";
                return std::nullopt;
            }
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
            std::string_view cookie_path = raw_cookie.Path();
            if (cookie_path.empty() || cookie_path[0] != '/') {
                //  Computing default-path
                cookie_path = path;
                if (cookie_path.empty() || cookie_path[0] != '/') {
                    cookie_path = "/";
                }
                const auto last_index = cookie_path.rfind('/');
                if (last_index != 0) {
                    cookie_path = cookie_path.substr(0, last_index);
                } else {
                    cookie_path = "/";
                }
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
            //  Preprocessing 
            const auto& lowered_scheme = ToLower(scheme);
            const bool secure_scheme = IsSecureScheme(lowered_scheme);
            if (result.name.starts_with("__Secure-")) {
                result.prefix_secure = true;
            } else if (result.name.starts_with("__Host-")) {
                result.prefix_host = true;
            }
            if ((result.prefix_secure && !result.secure) || (!secure_scheme && result.secure)) {
                // The __Secure- prefix only requires that the cookie be set secure
                LOG_WARNING() << "Failed security check on cookie: '" << raw_cookie.Name() << "'";
                return std::nullopt;
            }
            if (result.prefix_host) {
                //The __Host- prefix requires the cookie to be secure, have a "/" path
                //and not have a domain set.
                if (!(secure_scheme && result.secure && result.path == "/" && result.host_only)) {
                    LOG_WARNING() << "Failed host check on cookie: '" << raw_cookie.Name() << "'";
                    return std::nullopt;
                }
            }
            if (raw_cookie.IsHttpOnly() && !lowered_scheme.empty() && !lowered_scheme.starts_with("http")) {
                LOG_WARNING() << "Cookie was received not from http: '" << raw_cookie.Name() << "'";
                return std::nullopt;
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

    void InsertOrAssignCookieToMap(ValidatedCookie&& cookie) {
        //  A little optimization to exclude extra allocation instead of try_emplace way
        const auto& map_location = storage_.find(cookie.domain);
        if (map_location == storage_.end()) {
            std::string domain = cookie.domain;
            std::string cookie_name = cookie.name;
            storage_.emplace(std::move(domain), CookieNamesMap{{std::move(cookie_name), CookiesList{std::move(cookie)}}});
            return;
        }
        const auto& cookie_list_location = map_location->second.find(cookie.name);
        if (cookie_list_location == map_location->second.end()) {
            std::string cookie_name = cookie.name;
            map_location->second.emplace(std::move(cookie_name), CookiesList{std::move(cookie)});
            return;
        }
        auto& list = cookie_list_location->second;
        const auto& cookie_location = FindDuplicate(list, cookie);
        if (cookie_location != list.end()) {
            //  Preserving old creation time, needed for sorting output cookies
            cookie.creation_time = cookie_location->creation_time;
            *cookie_location = std::move(cookie);
            return;
            
        }
        list.push_back(std::move(cookie));
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
CookieJar::CookieJar(const CookieJar&) = default;
CookieJar::CookieJar(CookieJar&&) = default;
CookieJar& CookieJar::operator=(const CookieJar&) = default;
CookieJar& CookieJar::operator=(CookieJar&&) = default;

void CookieJar::Merge(CookieJar&& cookie_jar) {
    impl_->Merge(*cookie_jar.impl_);
}

void CookieJar::AddCookie(std::string_view url, server::http::Cookie&& cookie) {
    impl_->AddCookie(url, std::move(cookie));
}

std::optional<std::string> CookieJar::GetAnyCookieValue(std::string_view name) {
    return impl_->GetAnyCookieValue(name);
}

CookieJar::Cookies CookieJar::GetCookies(std::string_view url) {
    Cookies result;
    const auto& parsed_url = userver::http::DecomposeUrlIntoViews(url);
    const auto& lowered_domain = LowerDomainWithoutLeadingDot(parsed_url.host);
    //  Iteration without additional allocations
    std::string_view domain = lowered_domain;
    auto iterations =  std::ranges::count(domain, '.');
    for (;;) {
        if (domain.empty()) {
            break;
        }
        impl_->GetCookies(result, parsed_url.scheme, domain, parsed_url.path);
        if (iterations <= 1) {
            break;
        }
        --iterations;
        domain = domain.substr(domain.find('.') + 1);
    }
    if (result.size() > 1) {
        // In fact this optional but recommended step. Maybe add flag?
        // RFC 6265 5.4.1 - specifies order with remark:
        //   `Not all user agents sort the cookie-list in this order, but
        //   this order reflects common practice when this document was
        //   written, and, historically, there have been servers that
        //   (erroneously) depended on this order.`
        std::sort(result.begin(), result.end(), [](const CookieJar::Cookie& lhs, const CookieJar::Cookie& rhs) {
            if (lhs.path_length_ != rhs.path_length_) {
                return lhs.path_length_ > rhs.path_length_;
            }
            return lhs.creation_time_ < rhs.creation_time_;
        });
    }
    return result;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
