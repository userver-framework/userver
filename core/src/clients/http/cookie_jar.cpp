#include <userver/clients/http/cookie_jar.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <userver/utils/datetime.hpp>

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

// Every stored cookie-path that path-matches the request path, per RFC 6265                                                                                                                      
// §5.1.4 (Paths and Path-Match): the path itself plus each of its prefix                                                                                                                         
// directories down to "/". A request path that is empty or not absolute                                                                                                                          
// defaults to "/" (RFC 6265 §5.1.4, the default-path rule). Emitted                                                                                                                              
// longest-first so callers get the more specific matches earlier. 
std::vector<std::string> PathCandidates(std::string_view path) {
    std::vector<std::string> out;
    if (path.empty() || path.front() != '/') {
        out.emplace_back("/");
        return out;
    }
    std::string_view cur = path;
    while (true) {
        out.emplace_back(cur);
        const auto slash = cur.rfind('/');
        if (slash == 0) {
            if (cur.size() > 1) out.emplace_back("/");
            break;
        }
        cur = cur.substr(0, slash);
    }
    return out;
}

bool IsSecureScheme(std::string_view scheme) { return utils::StrIcaseEqual{}(scheme, "https"); }

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

struct CookieJar::Impl {
    using CookieKey = std::pair<std::string, std::string>;

    struct CookieKeyHash {
        std::size_t operator()(const CookieKey& key) const noexcept {
            const std::size_t h1 = domain_hash(key.first);
            const std::size_t h2 = path_hash(key.second);
            // boost::hash_combine mixing
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }

        // Held as members so the (randomly seeded) hash seed stays stable
        // across every lookup in a given table.
        utils::StrIcaseHash domain_hash;
        utils::StrCaseHash path_hash;
    };

    struct CookieKeyEqual {
        bool operator()(const CookieKey& lhs, const CookieKey& rhs) const noexcept {
            return domain_equal(lhs.first, rhs.first) && lhs.second == rhs.second;
        }

        utils::StrIcaseEqual domain_equal;
    };

    using CookieNameMap = std::unordered_map<std::string, server::http::Cookie, utils::StrCaseHash>;
    using Storage = std::unordered_map<CookieKey, CookieNameMap, CookieKeyHash, CookieKeyEqual>;

    Storage storage;
};

CookieJar::CookieJar() = default;
CookieJar::~CookieJar() = default;

CookieJar::CookieJar(const CookieJar&) = default;

CookieJar::CookieJar(CookieJar&&) noexcept = default;


void CookieJar::AddCookie(const std::string& domain, const std::string& path, Cookie&& cookie) {
    if (cookie.Domain().empty()) {
        cookie.SetDomain(domain);
    }
    if (cookie.Path().empty()) {
        cookie.SetPath(path);
    }

    Impl::CookieKey key{cookie.Domain(), cookie.Path()};

    // An expired cookie is a deletion request: drop any stored cookie sharing
    // this name+domain+path and store nothing.
    if (IsExpiredCookie(cookie)) {
        const auto it = impl_->storage.find(key);
        if (it != impl_->storage.end()) {
            it->second.erase(cookie.Name());
            if (it->second.empty()) {
                impl_->storage.erase(it);
            }
        }
        return;
    }

    // Re-setting an existing name+domain+path overwrites the previous value.
    std::string name = cookie.Name();
    impl_->storage[std::move(key)].insert_or_assign(std::move(name), std::move(cookie));
}

CookieJar::Cookies CookieJar::GetCookies(const std::string& domain, const std::string& path) {
    Cookies result;
    const auto domains = DomainCandidates(domain);
    const auto paths = PathCandidates(path);

    for (const auto& p : paths) {
        for (const auto& d : domains) {
            const auto it = impl_->storage.find(Impl::CookieKey{d, p});
            if (it == impl_->storage.end()) continue;
            for (const auto& [name, cookie] : it->second) {
                result.push_back(cookie);
            }
        }
    }

    return result;
}

}  // namespace clients::http

USERVER_NAMESPACE_END
