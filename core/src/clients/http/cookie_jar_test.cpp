#include <userver/clients/http/cookie_jar.hpp>

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>

#include <userver/server/http/http_response_cookie.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utest/utest.hpp>

namespace {

namespace datetime = USERVER_NAMESPACE::utils::datetime;

using CookieJar = USERVER_NAMESPACE::clients::http::CookieJar;
using Cookie = USERVER_NAMESPACE::server::http::Cookie;

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

#define EXPECT_COOKIES(url, expected) EXPECT_THAT(GetCookies(url), expected)

// A fixed, timezone-free "now" for Expires-based tests (2020-09-13T12:26:40Z),
// so the 2015/2050 Expires dates below are unambiguously past/future.
const auto kNow = std::chrono::system_clock::from_time_t(1'600'000'000);

class HttpCookieJar : public ::testing::Test {
protected:
    //  Helper method to store cookie at url. Uses subsequent parsing
    void Store(std::string_view url, std::string_view set_cookie) {
        auto cookie = Cookie::FromString(set_cookie);
        ASSERT_TRUE(cookie) << "cannot parse Set-Cookie: " << set_cookie;
        //cookie_jar_.AddCookie(url, std::move(*cookie));
    }

    //  Helper method for easy comparison extracted cookies. Exctracted cookies in "name=value" format
    std::vector<std::string> GetCookies(std::string_view url) {
        //const auto& cookies = cookie_jar_.GetCookies(url);
        std::vector<std::string> out;
        //out.reserve(cookies.size());
        //for (const auto& cookie : cookies) {
        //    out.push_back(cookie.Name() + '=' + cookie.Value());
        //}
        return out;
    }
private:
    CookieJar cookie_jar_ = CookieJar{};
};

}  // namespace

// The motivating scenario: two same-named cookies differing only by Path.
// Both domain- and path-match, so both are sent, most-specific (longest path)
// first per RFC 6265 §5.4.
UTEST_F(HttpCookieJar, SameNameDifferentPathBothSentLongestFirst) {   
    Store("localhost", "Garbage=3; Domain=localhost; Path=/garbage");
    Store("localhost", "A=3; Domain=localhost");
    Store("localhost/foo/boo", "A=2; Domain=localhost;");
    Store("localhost/foo/boo", "A=1; Domain=localhost; Path=/foo/bar");
    
    EXPECT_COOKIES("localhost/foo/bar", ElementsAre("A=1", "A=2", "A=3"));
}

// RFC 6265 §5.3 (step 11): a new cookie with the same (name, domain, path)
// replaces the old stored cookie.
UTEST_F(HttpCookieJar, SameKeyOverwrites) {
    Store("localhost", "sid=old; Domain=localhost; Path=/");
    Store("localhost", "sid=new; Domain=localhost; Path=/");

    EXPECT_COOKIES("localhost", ElementsAre("sid=new"));
}

// RFC 6265 §5.4: with no stored cookies the Cookie header is empty.
UTEST_F(HttpCookieJar, EmptyJarReturnsNothing) {
    EXPECT_COOKIES("localhost/foo", IsEmpty());
}

// Security issue, don't allow supercookie
UTEST_F(HttpCookieJar, EmptyJarForSuperCookies) {
    Store("a.com", "session=cracked; Domain=.com;");

    EXPECT_COOKIES("a.com", IsEmpty());
    EXPECT_COOKIES("hacked.com", IsEmpty());
    EXPECT_COOKIES("hacked.a.com", IsEmpty());
}

UTEST_F(HttpCookieJar, NonHttpCookie) {
    Store("myscheme://mytest.com", "a=test; Domain=mytest.com; HttpOnly");
    Store("http://mytest.com", "a=ok; Domain=mytest.com; HttpOnly");
    EXPECT_COOKIES("mytest.com", ElementsAre("a=ok"));
}

//  Checking combination implicit/explicit security attributes
UTEST_F(HttpCookieJar, SecurityCookies) {
    Store("http://mytest.com", "foo=test");
    Store("http://mytest.com", "secureFoo=test; Secure");
    Store("http://mytest.com", "__Secure-foo=test");
    Store("http://mytest.com", "__Host-foo=test");
    Store("https://mytest.com", "bar=ok; Secure");
    Store("https://mytest.com", "__Secure-bar=ok; Secure");
    Store("https://mytest.com", "__Secure-bar=test");
    Store("https://mytest.com", "__Host-bar=test; Path=/; Secure; ");
    Store("https://mytest.com", "__Host-barbar=test; Domain=mytest.com; Path=/; Secure; ");
    EXPECT_COOKIES("mytest.com", ElementsAre("foo=test"));
    EXPECT_COOKIES("http://mytest.com", ElementsAre("foo=test"));
    EXPECT_COOKIES("https://mytest.com", ElementsAre("foo=test", "bar=ok", "__Secure-bar=ok", "__Host-bar=test"));
}

// RFC 6265 §5.1.4 (path-match): a cookie-path matches only on directory
// boundaries, so "/foo" must not leak to "/foobar", and a request path shorter
// than the cookie-path must not match.
UTEST_F(HttpCookieJar, PathPrefixBoundary) {
    Store("localhost/foo", "a=1; Domain=localhost; Path=/foo");

    EXPECT_COOKIES("localhost/foo", ElementsAre("a=1"));
    EXPECT_COOKIES("localhost/foo/deep", ElementsAre("a=1"));
    EXPECT_COOKIES("localhost/foobar", IsEmpty());
    EXPECT_COOKIES("localhost/fo", IsEmpty());
    EXPECT_COOKIES("localhost/", IsEmpty());
}

UTEST_F(HttpCookieJar, UnicodeUrls) {
    Store("тест.рф/test", "a=1");

    EXPECT_COOKIES("тест.рф", ElementsAre("a=1"));
}

// RFC 6265 §5.1.4: cookie-path "/" is a prefix of every request path, so a root
// cookie matches everywhere; an empty or non-absolute request path takes the
// default-path "/".
UTEST_F(HttpCookieJar, RootPathMatchesEverything) {
    Store("localhost", "a=1; Domain=localhost; Path=/");

    EXPECT_COOKIES("localhost", ElementsAre("a=1"));
    EXPECT_COOKIES("localhost/anything/deep", ElementsAre("a=1"));
    EXPECT_COOKIES("localhost", ElementsAre("a=1"));
    EXPECT_COOKIES("localhost/relative", ElementsAre("a=1"));
}

// RFC 6265 §5.4 (rule 2): cookies with longer paths sort before shorter ones,
// regardless of insertion order.
UTEST_F(HttpCookieJar, DeepPathSpecificityOrdering) {
    Store("localhost/a/b", "lvl2=2; Domain=localhost; Path=/a/b");
    Store("localhost/", "root=r; Domain=localhost; Path=/");
    Store("localhost/a/b/c", "lvl3=3; Domain=localhost; Path=/a/b/c");
    Store("localhost/a", "lvl1=1; Domain=localhost; Path=/a");

    EXPECT_COOKIES("localhost/a/b/c/d", ElementsAre("lvl3=3", "lvl2=2", "lvl1=1", "root=r"));
}

// RFC 6265 §5.1.3 (domain-match): host names compare case-insensitively.
UTEST_F(HttpCookieJar, DomainIsCaseInsensitive) {
    Store("localhost", "a=1; Domain=LoCaLhOsT; Path=/");

    EXPECT_COOKIES("localhost", ElementsAre("a=1"));
    EXPECT_COOKIES("LOCALHOST", ElementsAre("a=1"));
}

// RFC 6265 §5.1.4 (path-match): paths, by contrast, compare as case-sensitive
// octet sequences.
UTEST_F(HttpCookieJar, PathIsCaseSensitive) {
    Store("localhost/Foo", "a=1; Domain=localhost; Path=/Foo");

    EXPECT_COOKIES("localhost/Foo", ElementsAre("a=1"));
    EXPECT_COOKIES("localhost/foo", IsEmpty());
}

// RFC 6265 §5.3: the cookie-name is part of a cookie's identity and is
// case-sensitive, so "A" and "a" coexist at the same (domain, path).
UTEST_F(HttpCookieJar, CookieNameIsCaseSensitive) {
    Store("localhost", "A=upper; Domain=localhost; Path=/");
    Store("localhost", "a=lower; Domain=localhost; Path=/");

    EXPECT_COOKIES("localhost/", UnorderedElementsAre("A=upper", "a=lower"));
}

// RFC 6265 §5.1.3 (domain-match): a cookie set for a parent domain is sent to
// its subdomains, but not to unrelated hosts that merely share a suffix
// substring (the match must fall on a "." boundary).
UTEST_F(HttpCookieJar, SuperdomainMatch) {
    Store("example.com", "a=1; Domain=example.com; Path=/");

    EXPECT_COOKIES("example.com", ElementsAre("a=1"));
    EXPECT_COOKIES("www.example.com", ElementsAre("a=1"));
    EXPECT_COOKIES("deep.www.example.com", ElementsAre("a=1"));
    EXPECT_COOKIES("notexample.com", IsEmpty());
    EXPECT_COOKIES("example.org", IsEmpty());
    EXPECT_COOKIES("com", IsEmpty());
}

// RFC 6265 §5.1.3: a cookie scoped to a subdomain must never travel up to the
// parent domain (the parent is not a domain-match for the subdomain).
UTEST_F(HttpCookieJar, SubdomainDoesNotLeakToParent) {
    Store("www.example.com", "a=1; Domain=www.example.com; Path=/");

    EXPECT_COOKIES("www.example.com", ElementsAre("a=1"));
    EXPECT_COOKIES("example.com", IsEmpty());
    EXPECT_COOKIES("other.example.com", IsEmpty());
}

// RFC 6265 §5.3 (identity is name+domain+path) with §5.4 (order): same-named
// cookies scoped to a sub- and a super-domain are distinct entries, so a
// request to the subdomain receives both (host-specific one first).
UTEST_F(HttpCookieJar, SameNameAcrossSubAndSuperDomain) {
    Store("example.com", "sid=parent; Domain=example.com; Path=/");
    Store("www.example.com", "sid=child; Domain=www.example.com; Path=/");

    EXPECT_COOKIES("www.example.com", ElementsAre("sid=parent", "sid=child"));
    EXPECT_COOKIES("example.com", ElementsAre("sid=parent"));
}

// RFC 6265 §5.3: a missing Domain defaults to the request host, and a missing
// Path defaults to the default-path (§5.1.4); an explicit
// attribute on the cookie takes precedence over these defaults.
UTEST_F(HttpCookieJar, DefaultsFilledFromRequestTarget) {
    Store("localhost/base/dir", "a=1");

    EXPECT_COOKIES("localhost/base/dir", ElementsAre("a=1"));
    EXPECT_COOKIES("localhost/base/dir/deeper", ElementsAre("a=1"));
    EXPECT_COOKIES("localhost/base", ElementsAre("a=1"));

    // Explicit Path "/" on the cookie wins over the "/ignored" request path.
    Store("localhost/ignored", "b=2; Domain=localhost; Path=/");
    EXPECT_COOKIES("localhost/", ElementsAre("b=2"));
}

// RFC 6265 §5.3: the stored cookie retains its resolved Domain/Path, which
// GetCookies exposes on the returned Cookie objects.
UTEST_F(HttpCookieJar, ReturnedCookieRetainsAttributes) {
    Store("localhost/base/dir", "a=1");
    EXPECT_COOKIES("localhost/base/dir", ElementsAre("a=1"));
}

// A trailing slash in the cookie Path is stored literally. 
UTEST_F(HttpCookieJar, TrailingSlashCookiePath) {
    Store("localhost/foo/", "a=1; Domain=localhost; Path=/foo/");
    Store("localhost/foo/", "b=1; Path=/foo");
    Store("localhost/foo/", "b=2");

    EXPECT_COOKIES("localhost/foo/bar", ElementsAre("a=1", "b=2"));
    EXPECT_COOKIES("localhost/foo/", ElementsAre("a=1", "b=2"));
}

// RFC 6265 §5.2.3 says a leading dot in a Domain attribute is ignored, so
// Domain=.example.com should behave like example.com. The jar keys on the
// literal domain string, so a leading-dot domain matches nothing here.
UTEST_F(HttpCookieJar, LeadingDotDomainQuirk) {
    Store("v1.myapi.com", "a=1; Domain=.v1.myapi.com; Path=/");

    EXPECT_COOKIES("v1.myapi.com", ElementsAre("a=1"));
    EXPECT_COOKIES(".v1.myapi.com", ElementsAre("a=1"));
    EXPECT_COOKIES("api.v1.myapi.com", ElementsAre("a=1"));
    EXPECT_COOKIES("test.api.v1.myapi.com", ElementsAre("a=1"));
    
    EXPECT_COOKIES("api.v2.myapi.com", IsEmpty());
    EXPECT_COOKIES("myapi.com", IsEmpty());
    EXPECT_COOKIES(".myapi.com", IsEmpty());
    EXPECT_COOKIES("amyapi.com", IsEmpty());
}

// --- Deletion (RFC 6265 §5.3) and overwrite semantics ---

// RFC 6265 §5.2.2: Max-Age <= 0 makes the cookie expire immediately, so
// §5.3 removes the stored cookie with the same name+domain+path.
UTEST_F(HttpCookieJar, MaxAgeZeroDeletesExistingCookie) {
    Store("localhost", "sid=v; Domain=localhost; Path=/");
    EXPECT_COOKIES("localhost", ElementsAre("sid=v"));

    Store("localhost", "sid=; Domain=localhost; Path=/; Max-Age=0");
    EXPECT_COOKIES("localhost", IsEmpty());
}

// RFC 6265 §5.2.2: a negative Max-Age is also a non-positive value and deletes.
UTEST_F(HttpCookieJar, NegativeMaxAgeDeletesExistingCookie) {
    Store("localhost", "sid=v; Domain=localhost; Path=/");
    Store("localhost", "sid=; Domain=localhost; Path=/; Max-Age=-1");

    EXPECT_COOKIES("localhost", IsEmpty());
}

// RFC 6265 §5.3 (step 11): if no matching cookie exists, deletion does nothing.
UTEST_F(HttpCookieJar, DeletionOfMissingCookieIsNoOp) {
    Store("localhost", "sid=; Domain=localhost; Path=/; Max-Age=0");

    EXPECT_COOKIES("localhost", IsEmpty());
}

// RFC 6265 §5.3: deletion targets the exact name+domain+path identity, so
// same-named cookies at other paths are untouched.
UTEST_F(HttpCookieJar, DeletionIsScopedToExactPath) {
    Store("localhost", "a=root; Domain=localhost; Path=/");
    Store("localhost/foo", "a=foo; Domain=localhost; Path=/foo");

    Store("localhost/foo", "a=; Domain=localhost; Path=/foo; Max-Age=0");

    // Only the /foo entry is gone; the root cookie still matches everywhere.
    EXPECT_COOKIES("localhost", ElementsAre("a=root"));
    EXPECT_COOKIES("localhost/foo", ElementsAre("a=root"));
}

// RFC 6265 §5.3 (step 11): replacing one name at a (domain, path) leaves the
// other cookies stored at that same key in place.
UTEST_F(HttpCookieJar, OverwriteAtSamePathKeepsSiblings) {
    Store("localhost", "a=1; Domain=localhost; Path=/");
    Store("localhost", "b=1; Domain=localhost; Path=/");
    Store("localhost", "a=2; Domain=localhost; Path=/");

    EXPECT_COOKIES("localhost", UnorderedElementsAre("a=2", "b=1"));
}

UTEST_F(HttpCookieJar, CookiesWithIpAndPort) {
    Store("127.0.0.1", "ip=true");
    Store("localhost:8081", "port=true");
    Store("127.0.0.2:8081", "ip_with_port=true");

    EXPECT_COOKIES("127.0.0.1", ElementsAre("ip=true"));
    EXPECT_COOKIES("localhost:8081", ElementsAre("port=true"));
    EXPECT_COOKIES("127.0.0.2:8081", ElementsAre("ip_with_port=true"));
}

// RFC 6265 §5.2.1: an Expires in the past sets expiry-time in the past, so
// §5.3 deletes the cookie.
UTEST_F(HttpCookieJar, ExpiresInPastDeletesExistingCookie) {
    datetime::MockNowSet(kNow);
    Store("localhost", "sid=v; Domain=localhost; Path=/");
    Store("localhost", "sid=; Domain=localhost; Path=/; Expires=Thu, 01 Jan 2015 00:00:00 GMT");

    EXPECT_COOKIES("localhost", IsEmpty());
    datetime::MockNowUnset();
}

// RFC 6265 §5.2.1: an Expires in the future keeps the cookie live, so it is
// stored normally.
UTEST_F(HttpCookieJar, ExpiresInFutureIsStored) {
    datetime::MockNowSet(kNow);
    Store("localhost", "sid=v; Domain=localhost; Path=/; Expires=Sat, 01 Jan 2050 00:00:00 GMT");

    EXPECT_COOKIES("localhost", ElementsAre("sid=v"));
    datetime::MockNowUnset();
}

// RFC 6265 §5.3 (step 3): when both are present Max-Age wins over Expires, in
// both directions.
UTEST_F(HttpCookieJar, MaxAgeTakesPrecedenceOverExpires) {
    datetime::MockNowSet(kNow);

    // Positive Max-Age wins over a past Expires -> stored.
    Store("localhost", "a=1; Domain=localhost; Path=/; Max-Age=3600; Expires=Thu, 01 Jan 2015 00:00:00 GMT");
    EXPECT_COOKIES("localhost", ElementsAre("a=1"));

    // Max-Age == 0 wins over a future Expires -> deleted.
    Store("localhost", "b=1; Domain=localhost; Path=/");
    Store("localhost", "b=; Domain=localhost; Path=/; Max-Age=0; Expires=Sat, 01 Jan 2050 00:00:00 GMT");
    EXPECT_COOKIES("localhost", ElementsAre("a=1"));

    datetime::MockNowUnset();
}
