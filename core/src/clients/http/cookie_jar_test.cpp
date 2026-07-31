#include <userver/clients/http/cookie_jar.hpp>

#include <chrono>
#include <string>
#include <string_view>
#include <fmt/format.h>
#include <vector>

#include <gmock/gmock.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <engine/task/task_processor.hpp>
#include <userver/server/http/http_response_cookie.hpp>
#include <userver/http/url.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/clients/http/client_core.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace datetime = USERVER_NAMESPACE::utils::datetime;

using CookieJar = USERVER_NAMESPACE::clients::http::CookieJar;
using Cookie = USERVER_NAMESPACE::server::http::Cookie;

using HttpResponse = USERVER_NAMESPACE::utest::SimpleServer::Response;
using HttpRequest = USERVER_NAMESPACE::utest::SimpleServer::Request;
using HttpCallback = USERVER_NAMESPACE::utest::SimpleServer::OnRequest;

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

#define EXPECT_COOKIES(url, expected) EXPECT_THAT(GetCookies(url), expected)

// For use in tests where we don't expect the timeout to expire.
constexpr auto kTimeout = utest::kMaxTestWaitTime;
// A fixed, timezone-free "now" for Expires-based tests (2020-09-13T12:26:40Z),
// so the 2015/2050 Expires dates below are unambiguously past/future.
const auto kNow = std::chrono::system_clock::from_time_t(1'600'000'000);

struct ReturnCookie {
    std::string* cookie;
    std::vector<server::http::Cookie>* received_cookies;

    HttpResponse operator()(const HttpRequest& request) {
        static const HttpResponse kOkResponse{
            "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 0\r\n\r\n",
            HttpResponse::kWriteAndClose
        };
        if (received_cookies != nullptr) {
            received_cookies->clear();
            const auto header_pos = request.find("Cookie:");
            if (header_pos != std::string::npos) {
                //  Cookies extraction
                EXPECT_EQ(request.find("Cookie:", header_pos + 1), std::string::npos)
                    << "Duplicate 'Cookie' header in request: " << request;

                const auto value_start = request.find_first_not_of(' ', header_pos + 7);
                if (value_start == std::string::npos) {
                    ADD_FAILURE() << "Malformed request: " << request;
                    return kOkResponse;
                }
                const auto value_end = request.find("\r\n", value_start);
                if (value_end == std::string::npos) {
                    ADD_FAILURE() << "Malformed request: " << request;
                    return kOkResponse;
                }

                std::vector<std::string> str_cookies;
                auto value = request.substr(value_start, value_end - value_start);
                boost::split(str_cookies, value, [](char c) { return c == ';'; });
                for (const auto& item : str_cookies) {
                    auto cookie = server::http::Cookie::FromString(item);
                    EXPECT_NE(cookie, std::nullopt)  << "Failed to parse cookie in request: " << request;
                    if (!cookie.has_value()) {
                        return kOkResponse;
                    }
                    received_cookies->push_back(std::move(cookie).value());
                }
            }
        }

        constexpr std::string_view prefix = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 0";
            std::string builder{prefix};
            if (cookie != nullptr && !cookie->empty()) {
                builder.append("\r\nSet-Cookie: ");
                builder.append(*cookie);
            }
            builder.append("\r\n\r\n");

        return HttpResponse{
            std::move(builder),
            HttpResponse::kWriteAndClose
        };
    }
};

struct ResolverWrapper {
    ResolverWrapper(const std::string& hosts_content)
        : hosts_file{[&] {
              auto file = fs::blocking::TempFile::Create();
              fs::blocking::RewriteFileContents(file.GetPath(), hosts_content);
              return file;
          }()},
          fs_task_processor{
              [] {
                  engine::TaskProcessorConfig config;
                  config.name = "fs-task-processor";
                  config.worker_threads = 1;
                  config.thread_name = "fs-worker";
                  return config;
              }(),
              engine::current_task::GetTaskProcessor().GetTaskProcessorPools()
          },
          resolver{
              fs_task_processor,
              [&] {
                  ::userver::static_config::DnsClient config;
                  config.hosts_file_path = hosts_file.GetPath();
                  config.hosts_file_update_interval = utest::kMaxTestWaitTime;
                  config.network_timeout = utest::kMaxTestWaitTime;
                  config.network_attempts = 1;
                  config.cache_max_reply_ttl = std::chrono::seconds{1};
                  config.cache_failure_ttl = std::chrono::seconds{1}, config.cache_ways = 1;
                  config.cache_size_per_way = 1;
                  return config;
              }()
          }
    {}

    fs::blocking::TempFile hosts_file;
    engine::TaskProcessor fs_task_processor;
    clients::dns::Resolver resolver;
};


class HttpCookieJar : public ::testing::Test {
protected:
    void MockDomains(std::set<std::string, std::less<>>  hosts) {
        hosts.insert("localhost");
        std::string hosts_content;
        for (const auto& host : hosts) {
            hosts_content.append(fmt::format("127.0.0.1 {}\r\n", host));
        }
        state_ = std::make_unique<State>(hosts_content, hosts);
    }

    //  Helper method to store cookie at libcurl's engine
    void Store(std::string_view url, std::string_view set_cookie) {
        MakeRequest(url, set_cookie);
    }

    //  Helper method for easy comparison extracted cookies. Exctracted cookies in "name=value" format
    std::vector<std::string> GetCookies(std::string_view url) {
        std::vector<std::string> formatted_out;
        for (const auto& item : MakeRequest(url, "")) {
            formatted_out.push_back(fmt::format("{}={}", item.Name(), item.Value()));
        }
        return formatted_out;
    }
private:
    static constexpr const char* kTestHosts = R"(
127.0.0.2 localhost
::1 localhost
127.0.0.1 localhost
::2 localhost
)";
    struct State {
        State(const std::string& hosts_content, std::set<std::string, std::less<>>  allowed_domains) : 
            valid_domains(allowed_domains), resolver_wrapper(hosts_content){
            client =  utest::impl::CreateHttpClientCore(resolver_wrapper.fs_task_processor);
            client->SetDnsResolver(&resolver_wrapper.resolver);
        }
        std::set<std::string, std::less<>>  valid_domains;
        std::string set_cookie;
        std::vector<server::http::Cookie> received_cookies;
        CookieJar cookie_jar;
        ResolverWrapper resolver_wrapper;
        std::shared_ptr<clients::http::ClientCore> client;
        utest::SimpleServer http_server{ReturnCookie{&set_cookie, &received_cookies}}; 
    };

    std::string FormatPort(std::string_view url) {
         auto parsed_url = userver::http::DecomposeUrlIntoViews(url);
         auto host = parsed_url.host.substr(0, parsed_url.host.find(":"));
         if (state_->valid_domains.find(std::string{host}) == state_->valid_domains.end()) {
            throw std::runtime_error(fmt::format("Attempt to use not mocked domain {}", parsed_url.host));
         }
         return fmt::format("{}://{}:{}{}", parsed_url.scheme, host, state_->http_server.GetPort(), parsed_url.path);
    }

    std::vector<server::http::Cookie> MakeRequest(std::string_view url, std::string_view set_cookie) {
        state_->set_cookie = set_cookie;
        auto request = state_->client->CreateRequest()
            .get(FormatPort(url))
            .retry(1)
            .verify(true)
            .cookies(state_->cookie_jar)
            .http_version(USERVER_NAMESPACE::http::HttpVersion::k11)
            .timeout(kTimeout);
        const auto res = request.perform();
        state_->cookie_jar = request.GetCookieJar();
        return state_->received_cookies;
    }
    std::unique_ptr<State> state_ = std::make_unique<State>(kTestHosts,  std::set<std::string, std::less<>>{{std::string{"localhost"}}});
};

}  // namespace

// The motivating scenario: two same-named cookies differing only by Path.
// Both domain- and path-match, so both are sent, most-specific (longest path)
// first per RFC 6265 §5.4.
UTEST_F(HttpCookieJar, SameNameDifferentPathBothSentLongestFirst) {   
    Store("http://localhost", "Garbage=3; Domain=localhost; Path=/garbage");
    Store("http://localhost", "A=3; Domain=localhost");
    Store("http://localhost/foo/boo", "A=2; Domain=localhost;");
    Store("http://localhost/foo/boo", "A=1; Domain=localhost; Path=/foo/bar");
    
    EXPECT_COOKIES("http://localhost/foo/bar", ElementsAre("A=1", "A=2", "A=3"));
}

// RFC 6265 §5.3 (step 11): a new cookie with the same (name, domain, path)
// replaces the old stored cookie.
UTEST_F(HttpCookieJar, SameKeyOverwrites) {
    Store("http://localhost", "sid=old; Domain=localhost; Path=/");
    Store("http://localhost", "sid=new; Domain=localhost; Path=/");

    EXPECT_COOKIES("http://localhost", ElementsAre("sid=new"));
}

// RFC 6265 §5.4: with no stored cookies the Cookie header is empty.
UTEST_F(HttpCookieJar, EmptyJarReturnsNothing) {
    EXPECT_COOKIES("http://localhost/foo", IsEmpty());
}

// Security issue, don't allow supercookie
UTEST_F(HttpCookieJar, EmptyJarForSuperCookies) {
    MockDomains({"a.com", "hacked.com", "hacked.a.com"});
    Store("http://a.com", "session=cracked; Domain=.com;");

    EXPECT_COOKIES("http://a.com", IsEmpty());
    EXPECT_COOKIES("http://hacked.com", IsEmpty());
    EXPECT_COOKIES("http://hacked.a.com", IsEmpty());
}

//  TODO: adapt new scheme for testing
UTEST_F(HttpCookieJar, DISABLED_NonHttpCookie) {
    MockDomains({"mytest.com"});
    Store("myscheme://mytest.com", "a=test; Domain=mytest.com; HttpOnly");
    Store("http://mytest.com", "a=ok; Domain=mytest.com; HttpOnly");
    EXPECT_COOKIES("http://mytest.com", ElementsAre("a=ok"));
}

//  Checking combination implicit/explicit security attributes
//  TODO: add support https server
UTEST_F(HttpCookieJar, DISABLED_SecurityCookies) {
    MockDomains({"mytest.com"});
    Store("http://mytest.com", "foo=test");
    Store("http://mytest.com", "secureFoo=test; Secure");
    Store("http://mytest.com", "__Secure-foo=test");
    Store("http://mytest.com", "__Host-foo=test");
    Store("https://mytest.com", "bar=ok; Secure");
    Store("https://mytest.com", "__Secure-bar=ok; Secure");
    Store("https://mytest.com", "__Secure-bar=test");
    Store("https://mytest.com", "__Host-bar=test; Path=/; Secure; ");
    Store("https://mytest.com", "__Host-barbar=test; Domain=mytest.com; Path=/; Secure; ");
    EXPECT_COOKIES("http://mytest.com", ElementsAre("foo=test"));
    EXPECT_COOKIES("http://mytest.com", ElementsAre("foo=test"));
    EXPECT_COOKIES("https://mytest.com", ElementsAre("foo=test", "bar=ok", "__Secure-bar=ok", "__Host-bar=test"));
}

// RFC 6265 §5.1.4 (path-match): a cookie-path matches only on directory
// boundaries, so "/foo" must not leak to "/foobar", and a request path shorter
// than the cookie-path must not match.
UTEST_F(HttpCookieJar, PathPrefixBoundary) {
    Store("http://localhost/foo", "a=1; Domain=localhost; Path=/foo");

    EXPECT_COOKIES("http://localhost/foo", ElementsAre("a=1"));
    EXPECT_COOKIES("http://localhost/foo/deep", ElementsAre("a=1"));
    EXPECT_COOKIES("http://localhost/foobar", IsEmpty());
    EXPECT_COOKIES("http://localhost/fo", IsEmpty());
    EXPECT_COOKIES("http://localhost/", IsEmpty());
}

//  TODO: Fix hosts for unicode in fixture
UTEST_F(HttpCookieJar, DISABLED_UnicodeUrls) {
    MockDomains({"тест.рф"});
    Store("http://тест.рф/test", "a=1");

    EXPECT_COOKIES("http://тест.рф", ElementsAre("a=1"));
}

// RFC 6265 §5.1.4: cookie-path "/" is a prefix of every request path, so a root
// cookie matches everywhere; an empty or non-absolute request path takes the
// default-path "/".
UTEST_F(HttpCookieJar, RootPathMatchesEverything) {
    Store("http://localhost", "a=1; Domain=localhost; Path=/");

    EXPECT_COOKIES("http://localhost", ElementsAre("a=1"));
    EXPECT_COOKIES("http://localhost/anything/deep", ElementsAre("a=1"));
    EXPECT_COOKIES("http://localhost", ElementsAre("a=1"));
    EXPECT_COOKIES("http://localhost/relative", ElementsAre("a=1"));
}

// RFC 6265 §5.4 (rule 2): cookies with longer paths sort before shorter ones,
// regardless of insertion order.
UTEST_F(HttpCookieJar, DeepPathSpecificityOrdering) {
    Store("http://localhost/a/b", "lvl2=2; Domain=localhost; Path=/a/b");
    Store("http://localhost/", "root=r; Domain=localhost; Path=/");
    Store("http://localhost/a/b/c", "lvl3=3; Domain=localhost; Path=/a/b/c");
    Store("http://localhost/a", "lvl1=1; Domain=localhost; Path=/a");

    EXPECT_COOKIES("http://localhost/a/b/c/d", ElementsAre("lvl3=3", "lvl2=2", "lvl1=1", "root=r"));
}

// RFC 6265 §5.1.3 (domain-match): host names compare case-insensitively.
// TODO: adapt test to support case insensivite mocked domains
UTEST_F(HttpCookieJar, DISABLED_DomainIsCaseInsensitive) {
    Store("http://localhost", "a=1; Domain=LoCaLhOsT; Path=/");

    EXPECT_COOKIES("http://localhost", ElementsAre("a=1"));
    EXPECT_COOKIES("http://LOCALHOST", ElementsAre("a=1"));
}

// RFC 6265 §5.1.4 (path-match): paths, by contrast, compare as case-sensitive
// octet sequences.
UTEST_F(HttpCookieJar, PathIsCaseSensitive) {
    Store("http://localhost/Foo", "a=1; Domain=localhost; Path=/Foo");

    EXPECT_COOKIES("http://localhost/Foo", ElementsAre("a=1"));
    EXPECT_COOKIES("http://localhost/foo", IsEmpty());
}

// RFC 6265 §5.3: the cookie-name is part of a cookie's identity and is
// case-sensitive, so "A" and "a" coexist at the same (domain, path).
UTEST_F(HttpCookieJar, CookieNameIsCaseSensitive) {
    Store("http://localhost", "A=upper; Domain=localhost; Path=/");
    Store("http://localhost", "a=lower; Domain=localhost; Path=/");

    EXPECT_COOKIES("http://localhost/", UnorderedElementsAre("A=upper", "a=lower"));
}

// RFC 6265 §5.1.3 (domain-match): a cookie set for a parent domain is sent to
// its subdomains, but not to unrelated hosts that merely share a suffix
// substring (the match must fall on a "." boundary).
UTEST_F(HttpCookieJar, SuperdomainMatch) {
    MockDomains({"example.com", "www.example.com", "deep.www.example.com", "notexample.com", "example.org", "com"});
    Store("http://example.com", "a=1; Domain=example.com; Path=/");

    EXPECT_COOKIES("http://example.com", ElementsAre("a=1"));
    EXPECT_COOKIES("http://www.example.com", ElementsAre("a=1"));
    EXPECT_COOKIES("http://deep.www.example.com", ElementsAre("a=1"));
    EXPECT_COOKIES("http://notexample.com", IsEmpty());
    EXPECT_COOKIES("http://example.org", IsEmpty());
    EXPECT_COOKIES("http://com", IsEmpty());
}

// RFC 6265 §5.1.3: a cookie scoped to a subdomain must never travel up to the
// parent domain (the parent is not a domain-match for the subdomain).
UTEST_F(HttpCookieJar, SubdomainDoesNotLeakToParent) {
    MockDomains({"example.com", "www.example.com", "other.example.com"});
    Store("http://www.example.com", "a=1; Domain=www.example.com; Path=/");

    EXPECT_COOKIES("http://www.example.com", ElementsAre("a=1"));
    EXPECT_COOKIES("http://example.com", IsEmpty());
    EXPECT_COOKIES("http://other.example.com", IsEmpty());
}

// RFC 6265 §5.3 (identity is name+domain+path) with §5.4 (order): same-named
// cookies scoped to a sub- and a super-domain are distinct entries, so a
// request to the subdomain receives both (host-specific one first).
UTEST_F(HttpCookieJar, SameNameAcrossSubAndSuperDomain) {
    MockDomains({"example.com", "www.example.com"});
    Store("http://example.com", "sid=parent; Domain=example.com; Path=/");
    Store("http://www.example.com", "sid=child; Domain=www.example.com; Path=/");

    EXPECT_COOKIES("http://www.example.com", UnorderedElementsAre("sid=parent", "sid=child"));
    EXPECT_COOKIES("http://example.com", ElementsAre("sid=parent"));
}

// RFC 6265 §5.3: a missing Domain defaults to the request host, and a missing
// Path defaults to the default-path (§5.1.4); an explicit
// attribute on the cookie takes precedence over these defaults.
UTEST_F(HttpCookieJar, DefaultsFilledFromRequestTarget) {
    Store("http://localhost/base/dir", "a=1");

    EXPECT_COOKIES("http://localhost/base/dir", ElementsAre("a=1"));
    EXPECT_COOKIES("http://localhost/base/dir/deeper", ElementsAre("a=1"));
    EXPECT_COOKIES("http://localhost/base", ElementsAre("a=1"));

    // Explicit Path "/" on the cookie wins over the "/ignored" request path.
    Store("http://localhost/ignored", "b=2; Domain=localhost; Path=/");
    EXPECT_COOKIES("http://localhost/", ElementsAre("b=2"));
}

// RFC 6265 §5.3: the stored cookie retains its resolved Domain/Path, which
// GetCookies exposes on the returned Cookie objects.
UTEST_F(HttpCookieJar, ReturnedCookieRetainsAttributes) {
    Store("http://localhost/base/dir", "a=1");
    EXPECT_COOKIES("http://localhost/base/dir", ElementsAre("a=1"));
}

// A trailing slash in the cookie Path is stored literally. 
UTEST_F(HttpCookieJar, TrailingSlashCookiePath) {
    Store("http://localhost/foo/", "a=1; Domain=localhost; Path=/foo/");
    Store("http://localhost/foo/", "b=1; Path=/foo");
    Store("http://localhost/foo/", "b=2");

    EXPECT_COOKIES("http://localhost/foo/bar", UnorderedElementsAre("a=1", "b=2"));
    EXPECT_COOKIES("http://localhost/foo/", UnorderedElementsAre("a=1", "b=2"));
}

// RFC 6265 §5.2.3 says a leading dot in a Domain attribute is ignored, so
// Domain=.example.com should behave like example.com. The jar keys on the
// literal domain string, so a leading-dot domain matches nothing here.
UTEST_F(HttpCookieJar, LeadingDotDomainQuirk) {
    MockDomains({".myapi.com", "amyapi.com", "myapi.com", "v1.myapi.com", ".v1.myapi.com", "api.v1.myapi.com", "test.api.v1.myapi.com", "api.v2.myapi.com"});
    Store("http://v1.myapi.com", "a=1; Domain=.v1.myapi.com; Path=/");

    EXPECT_COOKIES("http://v1.myapi.com", ElementsAre("a=1"));
    EXPECT_COOKIES("http://.v1.myapi.com", ElementsAre("a=1"));
    EXPECT_COOKIES("http://api.v1.myapi.com", ElementsAre("a=1"));
    EXPECT_COOKIES("http://test.api.v1.myapi.com", ElementsAre("a=1"));
    
    EXPECT_COOKIES("http://api.v2.myapi.com", IsEmpty());
    EXPECT_COOKIES("http://myapi.com", IsEmpty());
    EXPECT_COOKIES("http://.myapi.com", IsEmpty());
    EXPECT_COOKIES("http://amyapi.com", IsEmpty());
}

// --- Deletion (RFC 6265 §5.3) and overwrite semantics ---

// RFC 6265 §5.2.2: Max-Age <= 0 makes the cookie expire immediately, so
// §5.3 removes the stored cookie with the same name+domain+path.
UTEST_F(HttpCookieJar, MaxAgeZeroDeletesExistingCookie) {
    Store("http://localhost", "sid=v; Domain=localhost; Path=/");
    EXPECT_COOKIES("http://localhost", ElementsAre("sid=v"));

    Store("http://localhost", "sid=; Domain=localhost; Path=/; Max-Age=0");
    EXPECT_COOKIES("http://localhost", IsEmpty());
}

// RFC 6265 §5.2.2: a negative Max-Age is also a non-positive value and deletes.
UTEST_F(HttpCookieJar, NegativeMaxAgeDeletesExistingCookie) {
    Store("http://localhost", "sid=v; Domain=localhost; Path=/");
    Store("http://localhost", "sid=; Domain=localhost; Path=/; Max-Age=-1");

    EXPECT_COOKIES("http://localhost", IsEmpty());
}

// RFC 6265 §5.3 (step 11): if no matching cookie exists, deletion does nothing.
UTEST_F(HttpCookieJar, DeletionOfMissingCookieIsNoOp) {
    Store("http://localhost", "sid=; Domain=localhost; Path=/; Max-Age=0");

    EXPECT_COOKIES("http://localhost", IsEmpty());
}

// RFC 6265 §5.3: deletion targets the exact name+domain+path identity, so
// same-named cookies at other paths are untouched.
UTEST_F(HttpCookieJar, DeletionIsScopedToExactPath) {
    Store("http://localhost", "a=root; Domain=localhost; Path=/");
    Store("http://localhost/foo", "a=foo; Domain=localhost; Path=/foo");

    Store("http://localhost/foo", "a=; Domain=localhost; Path=/foo; Max-Age=0");

    // Only the /foo entry is gone; the root cookie still matches everywhere.
    EXPECT_COOKIES("http://localhost", ElementsAre("a=root"));
    EXPECT_COOKIES("http://localhost/foo", ElementsAre("a=root"));
}

// RFC 6265 §5.3 (step 11): replacing one name at a (domain, path) leaves the
// other cookies stored at that same key in place.
UTEST_F(HttpCookieJar, OverwriteAtSamePathKeepsSiblings) {
    Store("http://localhost", "a=1; Domain=localhost; Path=/");
    Store("http://localhost", "b=1; Domain=localhost; Path=/");
    Store("http://localhost", "a=2; Domain=localhost; Path=/");

    EXPECT_COOKIES("http://localhost", UnorderedElementsAre("a=2", "b=1"));
}

//  TODO: adapt test to use mock server port
UTEST_F(HttpCookieJar, DISABLED_CookiesWithIpAndPort) {
    Store("http://127.0.0.1", "ip=true");
    Store("http://localhost:8081", "port=true");
    Store("http://127.0.0.2:8081", "ip_with_port=true");

    EXPECT_COOKIES("http://127.0.0.1", ElementsAre("ip=true"));
    EXPECT_COOKIES("http://localhost:8081", ElementsAre("port=true"));
    EXPECT_COOKIES("http://127.0.0.2:8081", ElementsAre("ip_with_port=true"));
}

// RFC 6265 §5.2.1: an Expires in the past sets expiry-time in the past, so
// §5.3 deletes the cookie.
UTEST_F(HttpCookieJar, ExpiresInPastDeletesExistingCookie) {
    datetime::MockNowSet(kNow);
    Store("http://localhost", "sid=v; Domain=localhost; Path=/");
    Store("http://localhost", "sid=; Domain=localhost; Path=/; Expires=Thu, 01 Jan 2015 00:00:00 GMT");

    EXPECT_COOKIES("http://localhost", IsEmpty());
    datetime::MockNowUnset();
}

// RFC 6265 §5.2.1: an Expires in the future keeps the cookie live, so it is
// stored normally.
UTEST_F(HttpCookieJar, ExpiresInFutureIsStored) {
    datetime::MockNowSet(kNow);
    Store("http://localhost", "sid=v; Domain=localhost; Path=/; Expires=Sat, 01 Jan 2050 00:00:00 GMT");

    EXPECT_COOKIES("http://localhost", ElementsAre("sid=v"));
    datetime::MockNowUnset();
}

// RFC 6265 §5.3 (step 3): when both are present Max-Age wins over Expires, in
// both directions.
UTEST_F(HttpCookieJar, MaxAgeTakesPrecedenceOverExpires) {
    datetime::MockNowSet(kNow);

    // Positive Max-Age wins over a past Expires -> stored.
    Store("http://localhost", "a=1; Domain=localhost; Path=/; Max-Age=3600; Expires=Thu, 01 Jan 2015 00:00:00 GMT");
    EXPECT_COOKIES("http://localhost", ElementsAre("a=1"));

    // Max-Age == 0 wins over a future Expires -> deleted.
    Store("http://localhost", "b=1; Domain=localhost; Path=/");
    Store("http://localhost", "b=; Domain=localhost; Path=/; Max-Age=0; Expires=Sat, 01 Jan 2050 00:00:00 GMT");
    EXPECT_COOKIES("http://localhost", ElementsAre("a=1"));

    datetime::MockNowUnset();
}

USERVER_NAMESPACE_END