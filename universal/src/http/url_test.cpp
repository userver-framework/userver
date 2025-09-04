#include <string_view>

#include <gtest/gtest.h>

#include <userver/http/url.hpp>
#include <utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

using http::UrlEncode;

namespace {

std::string UrlDecode(std::string_view range) { return http::impl::UrlDecode(utils::impl::InternalTag{}, range); }

}  // namespace

TEST(UrlEncode, Empty) { EXPECT_EQ("", UrlEncode("")); }

TEST(UrlEncode, Latin) {
    constexpr std::string_view str = "SomeText1234567890";
    EXPECT_EQ(str, UrlEncode(str));
}

TEST(UrlEncode, Special) {
    constexpr std::string_view str = "Text with spaces,?&=";
    EXPECT_EQ("Text%20with%20spaces%2C%3F%26%3D", UrlEncode(str));
}

TEST(UrlDecode, Empty) { EXPECT_EQ("", UrlDecode("")); }

TEST(UrlDecode, Latin) {
    constexpr std::string_view str = "SomeText1234567890";
    EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Special) {
    constexpr std::string_view decoded = "Text with spaces,?&=";
    constexpr std::string_view str = "Text%20with%20spaces%2C%3F%26%3D";
    EXPECT_EQ(decoded, UrlDecode(str));
}

TEST(UrlDecode, Truncated1) {
    constexpr std::string_view str = "%2";
    EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Truncated2) {
    constexpr std::string_view str = "%";
    EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Invalid) {
    constexpr std::string_view str = "%%111";
    EXPECT_EQ("Q11", UrlDecode(str));
}

TEST(MakeQuery, MultiArgs) {
    const http::MultiArgs multi_args = {
        {"arg", "123"},
        {"arg", "456"},
        {"text", "green apple"},
    };

    EXPECT_EQ("arg=123&arg=456&text=green%20apple", http::MakeQuery(multi_args));
}

TEST(MakeUrl, InitializerList) { EXPECT_EQ("path?a=b&c=d", http::MakeUrl("path", {{"a", "b"}, {"c", "d"}})); }

TEST(MakeUrl, InitializerList2) {
    std::string value = "12345";
    EXPECT_EQ("path?a=12345&c=d", http::MakeUrl("path", {{"a", value}, {"c", "d"}}));
}

TEST(MakeUrl, Multi) { EXPECT_EQ("path?a=b&a=c", http::MakeUrl("path", {}, {{"a", "b"}, {"a", "c"}})); }

TEST(MakeUrl, QueryAndMultiquery) {
    EXPECT_EQ("path?a=b&a=c&k=v", http::MakeUrl("path", {{"k", "v"}}, {{"a", "b"}, {"a", "c"}}));
}

TEST(MakeUrl, Empty) { EXPECT_EQ("path", http::MakeUrl("path", {})); }

TEST(MakeUrlWithPathArgs, Empty) {
    auto result = http::MakeUrlWithPathArgs("", {});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("", result.value());

    result = http::MakeUrlWithPathArgs("/api/v1/users", {});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users", result.value());
}

TEST(MakeUrlWithPathArgs, SimpleSubstitution) {
    auto result = http::MakeUrlWithPathArgs("/api/v1/users/{user_id}", {{"user_id", "123"}});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123", result.value());

    result =
        http::MakeUrlWithPathArgs("/api/v1/users/{user_id}/posts/{post_id}", {{"user_id", "123"}, {"post_id", "456"}});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123/posts/456", result.value());
}

TEST(MakeUrlWithPathArgs, SpecialCharacters) {
    const auto result = http::MakeUrlWithPathArgs("/api/v1/search/{query}", {{"query", "hello world & special chars"}});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/search/hello%20world%20%26%20special%20chars", result.value());
}

TEST(MakeUrlWithPathArgs, MissingParameter) {
    const auto result = http::MakeUrlWithPathArgs("/api/v1/users/{user_id}/posts/{post_id}", {{"user_id", "123"}});
    EXPECT_FALSE(result.has_value());
}

TEST(MakeUrlWithPathArgs, ExtraParameter) {
    const auto result =
        http::MakeUrlWithPathArgs("/api/v1/users/{user_id}", {{"user_id", "123"}, {"extra", "ignored"}});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123", result.value());
}

TEST(MakeUrlWithPathArgs, RepeatedParameter) {
    const auto result = http::MakeUrlWithPathArgs("/api/{param}/test/{param}", {{"param", "value"}});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/value/test/value", result.value());
}

TEST(MakeUrlWithPathArgs, InvalidTemplate) {
    const auto result = http::MakeUrlWithPathArgs("/api/{unclosed", {{"unclosed", "value"}});
    EXPECT_FALSE(result.has_value());
}

TEST(MakeUrlWithPathArgs, EmptyKey) {
    const auto result = http::MakeUrlWithPathArgs("/api/{}/test", {{"", "value"}});
    EXPECT_FALSE(result.has_value());
}

TEST(MakeUrlWithPathArgs, WithQueryArgs) {
    const auto result = http::MakeUrlWithPathArgs(
        "/api/v1/users/{user_id}", {{"user_id", "123"}}, http::Args{{"filter", "active"}, {"sort", "name"}}
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123?sort=name&filter=active", result.value());
}

TEST(MakeUrlWithPathArgs, WithUnorderedMapQueryArgs) {
    const auto result = http::MakeUrlWithPathArgs(
        "/api/v1/users/{user_id}",
        {{"user_id", "123"}},
        std::unordered_map<std::string, std::string>{{"page", "1"}, {"limit", "20"}}
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123?limit=20&page=1", result.value());
}

TEST(MakeUrlWithPathArgs, WithQueryArgsAndMultiArgs) {
    const http::MultiArgs multi_args = {{"tag", "new"}, {"tag", "featured"}};

    const auto result = http::MakeUrlWithPathArgs(
        "/api/v1/products/{category}", {{"category", "electronics"}}, http::Args{{"sort", "price"}}, multi_args
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/products/electronics?sort=price&tag=new&tag=featured", result.value());
}

TEST(MakeUrlWithPathArgs, WithInitializerListQueryArgs) {
    const auto result = http::MakeUrlWithPathArgs(
        "/api/v1/search/{term}", {{"term", "laptop"}}, {{"brand", "apple"}, {"price_max", "2000"}}
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/search/laptop?brand=apple&price_max=2000", result.value());
}

TEST(MakeUrlWithPathArgs, WithQueryArgsAndError) {
    const auto result =
        http::MakeUrlWithPathArgs("/api/v1/users/{unclosed", {{"unclosed", "value"}}, http::Args{{"param", "value"}});
    EXPECT_FALSE(result.has_value());
}

TEST(MakeUrlWithPathArgs, WithQueryArgsEmptyPathArgs) {
    const auto result =
        http::MakeUrlWithPathArgs("/api/v1/products", {}, http::Args{{"category", "electronics"}, {"sort", "price"}});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/products?sort=price&category=electronics", result.value());
}

TEST(MakeUrlWithPathArgs, WithQueryArgsEmptyQueryArgs) {
    const auto result = http::MakeUrlWithPathArgs("/api/v1/users/{user_id}", {{"user_id", "123"}}, http::Args{});
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123", result.value());
}

TEST(MakeUrlWithPathArgs, WithSpecialCharsInQueryArgs) {
    const auto result = http::MakeUrlWithPathArgs(
        "/api/v1/users/{user_id}", {{"user_id", "123"}}, http::Args{{"q", "special chars & symbols"}}
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123?q=special%20chars%20%26%20symbols", result.value());
}

TEST(MakeUrlWithPathArgs, WithMixedPathAndQueryParameters) {
    const auto result = http::MakeUrlWithPathArgs(
        "/api/{version}/users/{user_id}/posts",
        {{"version", "v2"}, {"user_id", "123"}},
        {{"status", "published"}, {"order", "latest"}}
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v2/users/123/posts?status=published&order=latest", result.value());
}

TEST(ExtractMetaTypeFromUrlViewView, Empty) { EXPECT_EQ("", http::ExtractMetaTypeFromUrlView("")); }

TEST(ExtractMetaTypeFromUrlView, NoQuery) {
    EXPECT_EQ("ya.ru", http::ExtractMetaTypeFromUrlView("ya.ru"));
    EXPECT_EQ("ya.ru/some/path", http::ExtractMetaTypeFromUrlView("ya.ru/some/path"));
    EXPECT_EQ("http://ya.ru/some/path", http::ExtractMetaTypeFromUrlView("http://ya.ru/some/path"));
    EXPECT_EQ("http://ya.ru/some/path#abc", http::ExtractMetaTypeFromUrlView("http://ya.ru/some/path#abc"));
}

TEST(ExtractMetaTypeFromUrlView, WithQuery) {
    EXPECT_EQ("ya.ru", http::ExtractMetaTypeFromUrlView("ya.ru?abc=cde"));
    EXPECT_EQ("ya.ru", http::ExtractMetaTypeFromUrlView("ya.ru?abc=cde&v=x"));
    EXPECT_EQ("https://ya.ru", http::ExtractMetaTypeFromUrlView("https://ya.ru?abc=cde&v=x"));
    EXPECT_EQ("https://ya.ru/some/path", http::ExtractMetaTypeFromUrlView("https://ya.ru/some/path?abc=cde&v=x"));
}

TEST(ExtractPathView, Basic) {
    EXPECT_EQ("", http::ExtractPathView("http://service.com"));
    EXPECT_EQ("/", http::ExtractPathView("http://service.com/"));
    EXPECT_EQ("", http::ExtractPathView("service.com"));
    EXPECT_EQ("/", http::ExtractPathView("service.com/"));
    EXPECT_EQ("/aaa/bbb", http::ExtractPathView("service.com/aaa/bbb"));
    EXPECT_EQ("/aaa/bbb/", http::ExtractPathView("service.com/aaa/bbb/"));
    EXPECT_EQ("/aaa/bbb/", http::ExtractPathView("service.com/aaa/bbb/?q=1&w=2"));
    EXPECT_EQ("/aaa/bbb", http::ExtractPathView("service.com/aaa/bbb?q=1&w=2"));
    EXPECT_EQ("/aaa/bbb/", http::ExtractPathView("service.com/aaa/bbb/#123"));
    EXPECT_EQ("/aaa/bbb", http::ExtractPathView("service.com/aaa/bbb#123"));
    EXPECT_EQ("/aaa/bbb/", http::ExtractPathView("http://service.com/aaa/bbb/?q=1&w=2#1231"));
    EXPECT_EQ("/aaa/bbb", http::ExtractPathView("http://service.com/aaa/bbb?q=1&w=2#1231"));
    EXPECT_EQ("/aaa/bbb/", http::ExtractPathView("http://service.com/aaa/bbb/#abc?q=1&w=2#1231"));
    EXPECT_EQ("/aaa/bbb", http::ExtractPathView("http://service.com/aaa/bbb#abc?q=1&w=2#1231"));
}

TEST(ExtractHostnameView, Basic) {
    EXPECT_EQ("service.com", http::ExtractHostnameView("http://service.com"));
    EXPECT_EQ("service.com", http::ExtractHostnameView("http://service.com/"));
    EXPECT_EQ("service.com", http::ExtractHostnameView("http://service.com/aaa"));
    EXPECT_EQ("service.com", http::ExtractHostnameView("service.com"));
    EXPECT_EQ("service.com", http::ExtractHostnameView("service.com/"));
    EXPECT_EQ("service.com", http::ExtractHostnameView("service.com/aaa/bbb"));

    EXPECT_EQ("service.com", http::ExtractHostnameView("http://user@service.com"));
    EXPECT_EQ("service.com", http::ExtractHostnameView("http://user:pass@service.com"));

    EXPECT_EQ("service.com", http::ExtractHostnameView("http://service.com:80"));
    EXPECT_EQ("service.com", http::ExtractHostnameView("http://service.com:80/"));

    EXPECT_EQ("127.0.0.1", http::ExtractHostnameView("http://127.0.0.1:80"));
    EXPECT_EQ("127.0.0.1", http::ExtractHostnameView("http://127.0.0.1"));

    EXPECT_EQ("[::1]", http::ExtractHostnameView("http://[::1]:80/"));
    EXPECT_EQ("[::1]", http::ExtractHostnameView("http://[::1]:80"));
    EXPECT_EQ("[::1]", http::ExtractHostnameView("http://[::1]"));
}

TEST(ExtractSchemeView, Basic) {
    EXPECT_EQ("http", http::ExtractSchemeView("http://service.com"));
    EXPECT_EQ("", http::ExtractSchemeView("service.com"));
}

TEST(ExtractFragmentView, Basic) {
    EXPECT_EQ("", http::ExtractFragmentView("http://service.com/a/b/c?q=1"));
    EXPECT_EQ("abc", http::ExtractFragmentView("http://service.com/a/b/c?q=1#abc"));
    EXPECT_EQ("", http::ExtractFragmentView("http://service.com/a/b/c?q=1#"));
}

TEST(ExtractQueryView, Basic) {
    EXPECT_EQ("q=1", http::ExtractQueryView("http://service.com/a/b/c?q=1"));
    EXPECT_EQ("q=1&w=2", http::ExtractQueryView("http://service.com/a/b/c?q=1&w=2#abc"));
    EXPECT_EQ("", http::ExtractQueryView("http://service.com/a/b/c"));
    EXPECT_EQ("", http::ExtractQueryView("http://service.com/a/b/c#abc"));
    EXPECT_EQ("q=1", http::ExtractQueryView("http://service.com?q=1"));
    EXPECT_EQ("q=1", http::ExtractQueryView("http://service.com/?q=1"));
    EXPECT_EQ("q=1", http::ExtractQueryView("http://service.com/#abc?q=1"));
}

TEST(DecomposeUrlIntoViews, Basic) {
    {
        std::string_view url = "http://service.com/a/b/c?q=1&w=2#abc";
        auto decomposed = http::DecomposeUrlIntoViews(url);
        EXPECT_EQ(decomposed.scheme, "http");
        EXPECT_EQ(decomposed.host, "service.com");
        EXPECT_EQ(decomposed.path, "/a/b/c");
        EXPECT_EQ(decomposed.query, "q=1&w=2");
        EXPECT_EQ(decomposed.fragment, "abc");
    }
    {
        std::string_view url = "http://service.com/?q=1&w=2#abc";
        auto decomposed = http::DecomposeUrlIntoViews(url);
        EXPECT_EQ(decomposed.scheme, "http");
        EXPECT_EQ(decomposed.host, "service.com");
        EXPECT_EQ(decomposed.path, "/");
        EXPECT_EQ(decomposed.query, "q=1&w=2");
        EXPECT_EQ(decomposed.fragment, "abc");
    }
    {
        std::string_view url = "http://service.com/?q=1&w=2";
        auto decomposed = http::DecomposeUrlIntoViews(url);
        EXPECT_EQ(decomposed.scheme, "http");
        EXPECT_EQ(decomposed.host, "service.com");
        EXPECT_EQ(decomposed.path, "/");
        EXPECT_EQ(decomposed.query, "q=1&w=2");
        EXPECT_EQ(decomposed.fragment, "");
    }
    {
        std::string_view url = "http://service.com";
        auto decomposed = http::DecomposeUrlIntoViews(url);
        EXPECT_EQ(decomposed.scheme, "http");
        EXPECT_EQ(decomposed.host, "service.com");
        EXPECT_EQ(decomposed.path, "");
        EXPECT_EQ(decomposed.query, "");
        EXPECT_EQ(decomposed.fragment, "");
    }
}

USERVER_NAMESPACE_END
