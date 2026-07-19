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

// A fixed, timezone-free "now" for Expires-based tests (2020-09-13T12:26:40Z),
// so the 2015/2050 Expires dates below are unambiguously past/future.
const auto kNow = std::chrono::system_clock::from_time_t(1'600'000'000);

// Parses a Set-Cookie header value and stores it in the jar as though it arrived
// on a request to (request_host, request_path) - mirroring the receive path in
// RequestState. A Set-Cookie omitting Domain/Path inherits those from the
// request target (RFC 6265 §5.3, §5.1.4 default-path).
void Store(CookieJar& jar, std::string_view request_host, std::string_view request_path, std::string_view set_cookie) {
    auto cookie = Cookie::FromString(set_cookie);
    ASSERT_TRUE(cookie) << "cannot parse Set-Cookie: " << set_cookie;
    jar.AddCookie(std::string{request_host}, std::string{request_path}, std::move(*cookie));
}

// "name=value" strings in the exact order GetCookies returned them.
std::vector<std::string> Pairs(const CookieJar::Cookies& cookies) {
    std::vector<std::string> out;
    out.reserve(cookies.size());
    for (const auto& cookie : cookies) {
        out.push_back(cookie.first + '=' + cookie.second);
    }
    return out;
}

}  // namespace

// The motivating scenario: two same-named cookies differing only by Path.
// Both domain- and path-match, so both are sent, most-specific (longest path)
// first per RFC 6265 §5.4.
UTEST(HttpCookieJar, SameNameDifferentPathBothSentLongestFirst) {
    CookieJar jar;
    Store(jar, "localhost", "/", "A=1; Domain=localhost; Path=/");
    Store(jar, "localhost", "/foo", "A=2; Domain=localhost; Path=/foo");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/foo/bar")), ElementsAre("A=2", "A=1"));
}

// RFC 6265 §5.3 (step 11): a new cookie with the same (name, domain, path)
// replaces the old stored cookie.
UTEST(HttpCookieJar, SameKeyOverwrites) {
    CookieJar jar;
    Store(jar, "localhost", "/", "sid=old; Domain=localhost; Path=/");
    Store(jar, "localhost", "/", "sid=new; Domain=localhost; Path=/");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("sid=new"));
}

// RFC 6265 §5.4: with no stored cookies the Cookie header is empty.
UTEST(HttpCookieJar, EmptyJarReturnsNothing) {
    CookieJar jar;
    EXPECT_THAT(jar.GetCookies("localhost", "/foo"), IsEmpty());
}

// RFC 6265 §5.1.4 (path-match): a cookie-path matches only on directory
// boundaries, so "/foo" must not leak to "/foobar", and a request path shorter
// than the cookie-path must not match.
UTEST(HttpCookieJar, PathPrefixBoundary) {
    CookieJar jar;
    Store(jar, "localhost", "/foo", "a=1; Domain=localhost; Path=/foo");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/foo")), ElementsAre("a=1"));
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/foo/deep")), ElementsAre("a=1"));
    EXPECT_THAT(jar.GetCookies("localhost", "/foobar"), IsEmpty());
    EXPECT_THAT(jar.GetCookies("localhost", "/fo"), IsEmpty());
    EXPECT_THAT(jar.GetCookies("localhost", "/"), IsEmpty());
}

// RFC 6265 §5.1.4: cookie-path "/" is a prefix of every request path, so a root
// cookie matches everywhere; an empty or non-absolute request path takes the
// default-path "/".
UTEST(HttpCookieJar, RootPathMatchesEverything) {
    CookieJar jar;
    Store(jar, "localhost", "/", "a=1; Domain=localhost; Path=/");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("a=1"));
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/anything/deep")), ElementsAre("a=1"));
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "")), ElementsAre("a=1"));
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "relative")), ElementsAre("a=1"));
}

// RFC 6265 §5.4 (rule 2): cookies with longer paths sort before shorter ones,
// regardless of insertion order.
UTEST(HttpCookieJar, DeepPathSpecificityOrdering) {
    CookieJar jar;
    Store(jar, "localhost", "/a/b", "lvl2=2; Domain=localhost; Path=/a/b");
    Store(jar, "localhost", "/", "root=r; Domain=localhost; Path=/");
    Store(jar, "localhost", "/a/b/c", "lvl3=3; Domain=localhost; Path=/a/b/c");
    Store(jar, "localhost", "/a", "lvl1=1; Domain=localhost; Path=/a");

    EXPECT_THAT(
        Pairs(jar.GetCookies("localhost", "/a/b/c/d")), ElementsAre("lvl3=3", "lvl2=2", "lvl1=1", "root=r")
    );
}

// RFC 6265 §5.1.3 (domain-match): host names compare case-insensitively.
UTEST(HttpCookieJar, DomainIsCaseInsensitive) {
    CookieJar jar;
    Store(jar, "localhost", "/", "a=1; Domain=LoCaLhOsT; Path=/");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("a=1"));
    EXPECT_THAT(Pairs(jar.GetCookies("LOCALHOST", "/")), ElementsAre("a=1"));
}

// RFC 6265 §5.1.4 (path-match): paths, by contrast, compare as case-sensitive
// octet sequences.
UTEST(HttpCookieJar, PathIsCaseSensitive) {
    CookieJar jar;
    Store(jar, "localhost", "/Foo", "a=1; Domain=localhost; Path=/Foo");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/Foo")), ElementsAre("a=1"));
    EXPECT_THAT(jar.GetCookies("localhost", "/foo"), IsEmpty());
}

// RFC 6265 §5.3: the cookie-name is part of a cookie's identity and is
// case-sensitive, so "A" and "a" coexist at the same (domain, path).
UTEST(HttpCookieJar, CookieNameIsCaseSensitive) {
    CookieJar jar;
    Store(jar, "localhost", "/", "A=upper; Domain=localhost; Path=/");
    Store(jar, "localhost", "/", "a=lower; Domain=localhost; Path=/");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), UnorderedElementsAre("A=upper", "a=lower"));
}

// RFC 6265 §5.1.3 (domain-match): a cookie set for a parent domain is sent to
// its subdomains, but not to unrelated hosts that merely share a suffix
// substring (the match must fall on a "." boundary).
UTEST(HttpCookieJar, SuperdomainMatch) {
    CookieJar jar;
    Store(jar, "example.com", "/", "a=1; Domain=example.com; Path=/");

    EXPECT_THAT(Pairs(jar.GetCookies("example.com", "/")), ElementsAre("a=1"));
    EXPECT_THAT(Pairs(jar.GetCookies("www.example.com", "/")), ElementsAre("a=1"));
    EXPECT_THAT(Pairs(jar.GetCookies("deep.www.example.com", "/")), ElementsAre("a=1"));
    EXPECT_THAT(jar.GetCookies("notexample.com", "/"), IsEmpty());
    EXPECT_THAT(jar.GetCookies("example.org", "/"), IsEmpty());
    EXPECT_THAT(jar.GetCookies("com", "/"), IsEmpty());
}

// RFC 6265 §5.1.3: a cookie scoped to a subdomain must never travel up to the
// parent domain (the parent is not a domain-match for the subdomain).
UTEST(HttpCookieJar, SubdomainDoesNotLeakToParent) {
    CookieJar jar;
    Store(jar, "www.example.com", "/", "a=1; Domain=www.example.com; Path=/");

    EXPECT_THAT(Pairs(jar.GetCookies("www.example.com", "/")), ElementsAre("a=1"));
    EXPECT_THAT(jar.GetCookies("example.com", "/"), IsEmpty());
    EXPECT_THAT(jar.GetCookies("other.example.com", "/"), IsEmpty());
}

// RFC 6265 §5.3 (identity is name+domain+path) with §5.4 (order): same-named
// cookies scoped to a sub- and a super-domain are distinct entries, so a
// request to the subdomain receives both (host-specific one first).
UTEST(HttpCookieJar, SameNameAcrossSubAndSuperDomain) {
    CookieJar jar;
    Store(jar, "example.com", "/", "sid=parent; Domain=example.com; Path=/");
    Store(jar, "www.example.com", "/", "sid=child; Domain=www.example.com; Path=/");

    EXPECT_THAT(Pairs(jar.GetCookies("www.example.com", "/")), ElementsAre("sid=child", "sid=parent"));
    EXPECT_THAT(Pairs(jar.GetCookies("example.com", "/")), ElementsAre("sid=parent"));
}

// RFC 6265 §5.3: a missing Domain defaults to the request host, and a missing
// Path defaults to the request-uri path (§5.1.4 default-path); an explicit
// attribute on the cookie takes precedence over these defaults.
UTEST(HttpCookieJar, DefaultsFilledFromRequestTarget) {
    CookieJar jar;
    Store(jar, "localhost", "/base/dir", "a=1");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/base/dir")), ElementsAre("a=1"));
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/base/dir/deeper")), ElementsAre("a=1"));
    EXPECT_THAT(jar.GetCookies("localhost", "/base"), IsEmpty());

    // Explicit Path "/" on the cookie wins over the "/ignored" request path.
    Store(jar, "localhost", "/ignored", "b=2; Domain=localhost; Path=/");
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("b=2"));
}

// RFC 6265 §5.3: the stored cookie retains its resolved Domain/Path, which
// GetCookies exposes on the returned Cookie objects.
UTEST(HttpCookieJar, ReturnedCookieRetainsAttributes) {
    CookieJar jar;
    Store(jar, "localhost", "/base/dir", "a=1");

    const auto cookies = jar.GetCookies("localhost", "/base/dir");
    ASSERT_EQ(cookies.size(), 1);
    EXPECT_EQ(cookies.front().first, "a");
    EXPECT_EQ(cookies.front().second, "1");
}

// --- Current-behavior quirks that deviate from a strict RFC 6265 reading ---
// These lock in today's behavior; revisit if the matching is made RFC-strict.

// A trailing slash in the cookie Path is stored literally. GetCookies matches
// only directory-boundary prefixes of the request path, so "/foo/" is not a
// candidate for request "/foo/bar" and the cookie is withheld - whereas
// RFC 6265 §5.1.4 path-match would accept it.
UTEST(HttpCookieJar, TrailingSlashCookiePathQuirk) {
    CookieJar jar;
    Store(jar, "localhost", "/foo/", "a=1; Domain=localhost; Path=/foo/");

    EXPECT_THAT(jar.GetCookies("localhost", "/foo/bar"), IsEmpty());
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/foo/")), ElementsAre("a=1"));
}

// RFC 6265 §5.2.3 says a leading dot in a Domain attribute is ignored, so
// Domain=.example.com should behave like example.com. The jar keys on the
// literal domain string, so a leading-dot domain matches nothing here.
UTEST(HttpCookieJar, LeadingDotDomainQuirk) {
    CookieJar jar;
    Store(jar, "example.com", "/", "a=1; Domain=.example.com; Path=/");

    EXPECT_THAT(jar.GetCookies("example.com", "/"), IsEmpty());
    EXPECT_THAT(jar.GetCookies("www.example.com", "/"), IsEmpty());
    // It only matches a request whose host equals the literal ".example.com".
    EXPECT_THAT(Pairs(jar.GetCookies(".example.com", "/")), ElementsAre("a=1"));
}

// --- Deletion (RFC 6265 §5.3) and overwrite semantics ---

// RFC 6265 §5.2.2: Max-Age <= 0 makes the cookie expire immediately, so
// §5.3 removes the stored cookie with the same name+domain+path.
UTEST(HttpCookieJar, MaxAgeZeroDeletesExistingCookie) {
    CookieJar jar;
    Store(jar, "localhost", "/", "sid=v; Domain=localhost; Path=/");
    ASSERT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("sid=v"));

    Store(jar, "localhost", "/", "sid=; Domain=localhost; Path=/; Max-Age=0");

    EXPECT_THAT(jar.GetCookies("localhost", "/"), IsEmpty());
}

// RFC 6265 §5.2.2: a negative Max-Age is also a non-positive value and deletes.
UTEST(HttpCookieJar, NegativeMaxAgeDeletesExistingCookie) {
    CookieJar jar;
    Store(jar, "localhost", "/", "sid=v; Domain=localhost; Path=/");

    Store(jar, "localhost", "/", "sid=; Domain=localhost; Path=/; Max-Age=-1");

    EXPECT_THAT(jar.GetCookies("localhost", "/"), IsEmpty());
}

// RFC 6265 §5.3 (step 11): if no matching cookie exists, deletion does nothing.
UTEST(HttpCookieJar, DeletionOfMissingCookieIsNoOp) {
    CookieJar jar;

    Store(jar, "localhost", "/", "sid=; Domain=localhost; Path=/; Max-Age=0");

    EXPECT_THAT(jar.GetCookies("localhost", "/"), IsEmpty());
}

// RFC 6265 §5.3: deletion targets the exact name+domain+path identity, so
// same-named cookies at other paths are untouched.
UTEST(HttpCookieJar, DeletionIsScopedToExactPath) {
    CookieJar jar;
    Store(jar, "localhost", "/", "a=root; Domain=localhost; Path=/");
    Store(jar, "localhost", "/foo", "a=foo; Domain=localhost; Path=/foo");

    Store(jar, "localhost", "/foo", "a=; Domain=localhost; Path=/foo; Max-Age=0");

    // Only the /foo entry is gone; the root cookie still matches everywhere.
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("a=root"));
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/foo")), ElementsAre("a=root"));
}

// RFC 6265 §5.3 (step 11): replacing one name at a (domain, path) leaves the
// other cookies stored at that same key in place.
UTEST(HttpCookieJar, OverwriteAtSamePathKeepsSiblings) {
    CookieJar jar;
    Store(jar, "localhost", "/", "a=1; Domain=localhost; Path=/");
    Store(jar, "localhost", "/", "b=1; Domain=localhost; Path=/");
    Store(jar, "localhost", "/", "a=2; Domain=localhost; Path=/");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), UnorderedElementsAre("a=2", "b=1"));
}

// RFC 6265 §5.2.1: an Expires in the past sets expiry-time in the past, so
// §5.3 deletes the cookie.
UTEST(HttpCookieJar, ExpiresInPastDeletesExistingCookie) {
    datetime::MockNowSet(kNow);

    CookieJar jar;
    Store(jar, "localhost", "/", "sid=v; Domain=localhost; Path=/");

    Store(jar, "localhost", "/", "sid=; Domain=localhost; Path=/; Expires=Thu, 01 Jan 2015 00:00:00 GMT");

    EXPECT_THAT(jar.GetCookies("localhost", "/"), IsEmpty());

    datetime::MockNowUnset();
}

// RFC 6265 §5.2.1: an Expires in the future keeps the cookie live, so it is
// stored normally.
UTEST(HttpCookieJar, ExpiresInFutureIsStored) {
    datetime::MockNowSet(kNow);

    CookieJar jar;
    Store(jar, "localhost", "/", "sid=v; Domain=localhost; Path=/; Expires=Sat, 01 Jan 2050 00:00:00 GMT");

    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("sid=v"));

    datetime::MockNowUnset();
}

// RFC 6265 §5.3 (step 3): when both are present Max-Age wins over Expires, in
// both directions.
UTEST(HttpCookieJar, MaxAgeTakesPrecedenceOverExpires) {
    datetime::MockNowSet(kNow);

    CookieJar jar;

    // Positive Max-Age wins over a past Expires -> stored.
    Store(jar, "localhost", "/", "a=1; Domain=localhost; Path=/; Max-Age=3600; Expires=Thu, 01 Jan 2015 00:00:00 GMT");
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("a=1"));

    // Max-Age == 0 wins over a future Expires -> deleted.
    Store(jar, "localhost", "/", "b=1; Domain=localhost; Path=/");
    Store(jar, "localhost", "/", "b=; Domain=localhost; Path=/; Max-Age=0; Expires=Sat, 01 Jan 2050 00:00:00 GMT");
    EXPECT_THAT(Pairs(jar.GetCookies("localhost", "/")), ElementsAre("a=1"));

    datetime::MockNowUnset();
}
