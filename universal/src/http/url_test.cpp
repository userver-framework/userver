#include <string_view>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/http/url.hpp>
#include <utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

using http::EncodeS3Key;
using http::ExtractFragment;
using http::ExtractHostname;
using http::ExtractMetaTypeFromUrl;
using http::ExtractPath;
using http::ExtractPathOnly;
using http::ExtractQuery;
using http::ExtractScheme;
using http::MakeQuery;
using http::MakeUrl;
using http::MakeUrlWithPathArgs;
using http::ToPunycodeAscii;
using http::UrlEncode;
using http::UrlEncodePathSegment;

namespace {

std::string UrlDecode(std::string_view range) { return http::impl::UrlDecode(utils::impl::InternalTag{}, range); }

}  // namespace

TEST(UrlEncode, Empty) { EXPECT_EQ("", UrlEncode("")); }

TEST(UrlEncode, Latin) {
    constexpr std::string_view str = "SomeText1234567890";
    EXPECT_EQ(str, UrlEncode(str));
}

TEST(UrlEncodePathSegment, Empty) { EXPECT_EQ("", UrlEncodePathSegment("")); }

TEST(UrlEncodePathSegment, Latin) {
    constexpr std::string_view str = "SomeText1234567890";
    EXPECT_EQ(str, UrlEncodePathSegment(str));
}

TEST(UrlEncodePathSegment, UnreservedChars) {
    // RFC 3986 unreserved: - _ . ~
    constexpr std::string_view str = "file-name_test.txt~backup";
    EXPECT_EQ(str, UrlEncodePathSegment(str));
}

TEST(UrlEncodePathSegment, PathSafeSpecialChars) {
    // Additional path-safe: $ & , : = @
    constexpr std::string_view str = "price$100&tax:50=total@rate";
    EXPECT_EQ(str, UrlEncodePathSegment(str));
}

TEST(UrlEncodePathSegment, SpacesAndSpecial) {
    constexpr std::string_view str = "file with spaces.txt";
    EXPECT_EQ("file%20with%20spaces.txt", UrlEncodePathSegment(str));
}

TEST(UrlEncodePathSegment, SlashShouldNotBeEncoded) {
    // Slash should be encoded in path segment context
    constexpr std::string_view str = "folder/file";
    EXPECT_EQ("folder%2Ffile", UrlEncodePathSegment(str));
}

TEST(UrlEncodePathSegment, QueryChars) {
    // ? and # should be encoded
    constexpr std::string_view str = "file?query#fragment";
    EXPECT_EQ("file%3Fquery%23fragment", UrlEncodePathSegment(str));
}

TEST(EncodeS3Key, Empty) { EXPECT_EQ("", EncodeS3Key("")); }

TEST(EncodeS3Key, SimpleKey) {
    constexpr std::string_view key = "simple-key.txt";
    EXPECT_EQ(key, EncodeS3Key(key));
}

TEST(EncodeS3Key, WithSpaces) {
    constexpr std::string_view key = "file with spaces.txt";
    EXPECT_EQ("file%20with%20spaces.txt", EncodeS3Key(key));
}

TEST(EncodeS3Key, WithSlashes) {
    constexpr std::string_view key = "folder/subfolder/file.txt";
    EXPECT_EQ("folder/subfolder/file.txt", EncodeS3Key(key));
}

TEST(EncodeS3Key, WithSlashesAndSpaces) {
    constexpr std::string_view key = "folder/file with spaces.txt";
    EXPECT_EQ("folder/file%20with%20spaces.txt", EncodeS3Key(key));
}

TEST(EncodeS3Key, ComplexKey) {
    constexpr std::string_view key = "path/to/my file (copy).txt";
    // Parentheses are encoded as they're not in our path-safe set (RFC 3986 unreserved + S3-safe)
    EXPECT_EQ("path/to/my%20file%20%28copy%29.txt", EncodeS3Key(key));
}

TEST(EncodeS3Key, SpecialCharsInSegments) {
    // Path-safe chars should be preserved, others encoded
    constexpr std::string_view key = "folder/file-name_with.dots~and$symbols&commas,colons:equals=at@sign";
    EXPECT_EQ("folder/file-name_with.dots~and$symbols&commas,colons:equals=at@sign", EncodeS3Key(key));
}

TEST(EncodeS3Key, LeadingSlash) {
    // Leading slash should be preserved as S3 path separator
    constexpr std::string_view key = "/leading/slash.txt";
    EXPECT_EQ("/leading/slash.txt", EncodeS3Key(key));
}

TEST(EncodeS3Key, MultipleSpacesInPath) {
    constexpr std::string_view key = "folder with spaces/file with spaces.txt";
    EXPECT_EQ("folder%20with%20spaces/file%20with%20spaces.txt", EncodeS3Key(key));
}

TEST(EncodeS3Key, UnicodeCharacters) {
    // Unicode characters should be percent-encoded
    // UTF-8 encoding of Cyrillic 'ф' (U+0444) is D1 84, 'а' (U+0430) is D0 B0, 'й' (U+0439) is D0 B9
    constexpr std::string_view key = "folder/файл.txt";
    auto result = EncodeS3Key(key);
    // Check that Cyrillic characters are percent-encoded
    EXPECT_EQ(result, "folder/%D1%84%D0%B0%D0%B9%D0%BB.txt");
    EXPECT_THAT(result, testing::HasSubstr("folder/"));
    EXPECT_THAT(result, testing::HasSubstr(".txt"));
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
    /// [MakeQuery MultiArgs]
    const http::MultiArgs multi_args = {
        {"arg", "123"},
        {"arg", "456"},
        {"text", "green apple"},
    };

    EXPECT_EQ("arg=123&arg=456&text=green%20apple", http::MakeQuery(multi_args));
    /// [MakeQuery MultiArgs]
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
    const auto
        result = http::MakeUrlWithPathArgs("/api/v1/users/{user_id}", {{"user_id", "123"}, {"extra", "ignored"}});
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
        "/api/v1/users/{user_id}",
        {{"user_id", "123"}},
        http::Args{{"filter", "active"}, {"sort", "name"}}
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123?sort=name&filter=active", result.value());
}

TEST(MakeUrlWithPathArgs, WithUnorderedMapQueryArgs) {
    /// [MakeUrlWithPathArgs unordered map query args]
    const auto result = http::MakeUrlWithPathArgs(
        "/api/v1/users/{user_id}",
        {{"user_id", "123"}},
        std::unordered_map<std::string, std::string>{{"page", "1"}, {"limit", "20"}}
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/users/123?limit=20&page=1", result.value());
    /// [MakeUrlWithPathArgs unordered map query args]
}

TEST(MakeUrlWithPathArgs, WithQueryArgsAndMultiArgs) {
    /// [MakeUrlWithPathArgs MultiArgs]
    const http::MultiArgs multi_args = {{"tag", "new"}, {"tag", "featured"}};

    const auto result = http::MakeUrlWithPathArgs(
        "/api/v1/products/{category}",
        {{"category", "electronics"}},
        http::Args{{"sort", "price"}},
        multi_args
    );
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("/api/v1/products/electronics?sort=price&tag=new&tag=featured", result.value());
    /// [MakeUrlWithPathArgs MultiArgs]
}

TEST(MakeUrlWithPathArgs, WithInitializerListQueryArgs) {
    const auto result = http::MakeUrlWithPathArgs(
        "/api/v1/search/{term}",
        {{"term", "laptop"}},
        {{"brand", "apple"}, {"price_max", "2000"}}
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
        "/api/v1/users/{user_id}",
        {{"user_id", "123"}},
        http::Args{{"q", "special chars & symbols"}}
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

TEST(UrlDocs, UrlEncodeExample) {
    /// [UrlEncode example]
    auto encoded = UrlEncode("hello world");
    EXPECT_EQ(encoded, "hello%20world");
    /// [UrlEncode example]
}

TEST(UrlDocs, UrlEncodePathSegmentExample) {
    /// [UrlEncodePathSegment example]
    auto encoded = UrlEncodePathSegment("file-name_with spaces.txt");
    EXPECT_EQ(encoded, "file-name_with%20spaces.txt");
    /// [UrlEncodePathSegment example]
}

TEST(UrlDocs, EncodeS3KeyExample) {
    /// [EncodeS3Key example]
    auto encoded = EncodeS3Key("folder/file with spaces.txt");
    EXPECT_EQ(encoded, "folder/file%20with%20spaces.txt");
    /// [EncodeS3Key example]
}

TEST(UrlDocs, MakeQueryExample) {
    /// [MakeQuery example]
    auto query = MakeQuery(http::Args{{"param", "value"}, {"filter", "active"}});
    EXPECT_TRUE(query == "param=value&filter=active" || query == "filter=active&param=value");
    /// [MakeQuery example]
}

TEST(UrlDocs, MakeQueryMultiArgsExample) {
    /// [MakeQuery MultiArgs example]
    http::MultiArgs args = {{"tag", "new"}, {"tag", "featured"}};
    auto query = MakeQuery(args);
    EXPECT_TRUE(query == "tag=new&tag=featured" || query == "tag=featured&tag=new");
    /// [MakeQuery MultiArgs example]
}

TEST(UrlDocs, MakeQueryunorderedmapExample) {
    /// [MakeQuery unordered_map example]
    auto query = MakeQuery(std::unordered_map<std::string, std::string>{{"page", "1"}, {"size", "10"}});
    EXPECT_TRUE(query == "page=1&size=10" || query == "size=10&page=1");
    /// [MakeQuery unordered_map example]
}

TEST(UrlDocs, MakeQueryinitializerlistExample) {
    /// [MakeQuery initializer list example]
    auto query = MakeQuery({{"sort", "date"}, {"order", "desc"}});
    EXPECT_TRUE(query == "sort=date&order=desc" || query == "order=desc&sort=date");
    /// [MakeQuery initializer list example]
}

TEST(UrlDocs, MakeUrlExample) {
    /// [MakeUrl example]
    auto url = MakeUrl("/api/users", http::Args{{"status", "active"}});
    EXPECT_EQ(url, "/api/users?status=active");
    /// [MakeUrl example]
}

TEST(UrlDocs, MakeUrlwithunorderedmapExample) {
    /// [MakeUrl with unordered_map example]
    auto url = MakeUrl("/api/products", std::unordered_map<std::string, std::string>{{"category", "electronics"}});
    EXPECT_EQ(url, "/api/products?category=electronics");
    /// [MakeUrl with unordered_map example]
}

TEST(UrlDocs, MakeUrlwithMultiArgsExample) {
    /// [MakeUrl with MultiArgs example]
    http::MultiArgs multi_args = {{"tag", "new"}, {"tag", "featured"}};
    auto url = MakeUrl("/api/products", http::Args{{"category", "electronics"}}, multi_args);
    EXPECT_EQ(url, "/api/products?category=electronics&tag=new&tag=featured");
    /// [MakeUrl with MultiArgs example]
}

TEST(UrlDocs, MakeUrlwithinitializerlistExample) {
    /// [MakeUrl with initializer list example]
    auto url = MakeUrl("/api/search", {{"q", "smartphone"}, {"sort", "relevance"}});
    EXPECT_EQ(url, "/api/search?q=smartphone&sort=relevance");
    /// [MakeUrl with initializer list example]
}

TEST(UrlDocs, MakeUrlWithPathArgsExample) {
    /// [MakeUrlWithPathArgs example]
    auto url = MakeUrlWithPathArgs("/api/v1/users/{user_id}", {{"user_id", "123"}});
    ASSERT_TRUE(url.has_value());
    EXPECT_EQ(*url, "/api/v1/users/123");
    /// [MakeUrlWithPathArgs example]
}

TEST(UrlDocs, MakeUrlWithPathArgswithqueryExample) {
    /// [MakeUrlWithPathArgs with query example]
    auto url = MakeUrlWithPathArgs("/api/v1/users/{user_id}", {{"user_id", "123"}}, http::Args{{"filter", "active"}});
    ASSERT_TRUE(url.has_value());
    EXPECT_EQ(*url, "/api/v1/users/123?filter=active");
    /// [MakeUrlWithPathArgs with query example]
}

TEST(UrlDocs, MakeUrlWithPathArgswithunorderedmapExample) {
    /// [MakeUrlWithPathArgs with unordered_map example]
    auto url = MakeUrlWithPathArgs(
        "/api/v1/users/{user_id}",
        {{"user_id", "123"}},
        std::unordered_map<std::string, std::string>{{"page", "1"}}
    );
    ASSERT_TRUE(url.has_value());
    EXPECT_EQ(*url, "/api/v1/users/123?page=1");
    /// [MakeUrlWithPathArgs with unordered_map example]
}

TEST(UrlDocs, MakeUrlWithPathArgswithMultiArgsExample) {
    /// [MakeUrlWithPathArgs with MultiArgs example]
    http::MultiArgs multi_args = {{"tag", "new"}, {"tag", "featured"}};
    auto url = MakeUrlWithPathArgs(
        "/api/v1/products/{category}",
        {{"category", "electronics"}},
        http::Args{{"sort", "price"}},
        multi_args
    );
    ASSERT_TRUE(url.has_value());
    EXPECT_EQ(*url, "/api/v1/products/electronics?sort=price&tag=new&tag=featured");
    /// [MakeUrlWithPathArgs with MultiArgs example]
}

TEST(UrlDocs, MakeUrlWithPathArgswithinitializerlistExample) {
    /// [MakeUrlWithPathArgs with initializer list example]
    auto url =
        MakeUrlWithPathArgs("/api/v1/search/{term}", {{"term", "laptop"}}, {{"brand", "apple"}, {"price_max", "2000"}});
    ASSERT_TRUE(url.has_value());
    EXPECT_EQ(*url, "/api/v1/search/laptop?brand=apple&price_max=2000");
    /// [MakeUrlWithPathArgs with initializer list example]
}

TEST(UrlDocs, ExtractMetaTypeFromUrlExample) {
    /// [ExtractMetaTypeFromUrl example]
    auto base = ExtractMetaTypeFromUrl("https://example.com/api/users?page=1&sort=name");
    EXPECT_EQ(base, "https://example.com/api/users");
    /// [ExtractMetaTypeFromUrl example]
}

TEST(UrlDocs, ExtractPathOnlyExample) {
    /// [ExtractPathOnly example]
    auto path = ExtractPath("https://example.com/api/users");
    EXPECT_EQ(path, "/api/users");

    auto path2 = ExtractPath("example.com/api/users?a=b");
    EXPECT_EQ(path2, "/api/users");
    /// [ExtractPathOnly example]
}

TEST(UrlDocs, ExtractPathOnlyExample2) {
    /// [ExtractPathOnly example 2]
    auto path = ExtractPathOnly("https://example.com/api/users");
    EXPECT_EQ(path, "/api/users");

    auto path2 = ExtractPathOnly("example.com/api/users?a=b");
    EXPECT_EQ(path2, "/api/users");
    /// [ExtractPathOnly example 2]
}

TEST(UrlDocs, ExtractHostnameExample) {
    /// [ExtractHostname example]
    auto host = ExtractHostname("https://example.com/api/users");
    EXPECT_EQ(host, "example.com");

    auto host2 = ExtractHostname("https://user:pass@example.com:8080/api");
    EXPECT_EQ(host2, "example.com");

    auto host3 = ExtractHostname("http://[::1]:8080/");
    EXPECT_EQ(host3, "[::1]");
    /// [ExtractHostname example]
}

TEST(UrlDocs, ExtractSchemeExample) {
    /// [ExtractScheme example]
    auto scheme = ExtractScheme("https://example.com/api/users");
    EXPECT_EQ(scheme, "https");

    auto scheme2 = ExtractScheme("http://user:pass@example.com:8080/api");
    EXPECT_EQ(scheme2, "http");

    auto scheme3 = ExtractScheme("ftp://[::1]:8080/");
    EXPECT_EQ(scheme3, "ftp");
    /// [ExtractScheme example]
}

TEST(UrlDocs, ExtractQueryExample) {
    /// [ExtractQuery example]
    auto query = ExtractQuery("https://example.com/api/users?q=1");
    EXPECT_EQ(query, "q=1");

    auto query2 = ExtractQuery("http://user:pass@example.com:8080/api");
    EXPECT_EQ(query2, "");

    auto query3 = ExtractQuery("ftp://[::1]:8080/?q=12&w=23");
    EXPECT_TRUE(query3 == "q=12&w=23" || query3 == "w=23&q=12");
    /// [ExtractQuery example]
}

TEST(UrlDocs, ExtractFragmentExample) {
    /// [ExtractFragment example]
    auto fragment = ExtractFragment("https://example.com/api/users?q=1");
    EXPECT_EQ(fragment, "");

    auto fragment2 = ExtractFragment("http://user:pass@example.com:8080/api#123");
    EXPECT_EQ(fragment2, "123");

    auto fragment3 = ExtractFragment("ftp://[::1]:8080/#123?q=12&w=23");
    EXPECT_EQ(fragment3, "123?q=12&w=23");

    auto fragment4 = ExtractFragment("ftp://[::1]:8080/?q=12&w=23#123");
    EXPECT_EQ(fragment4, "123");
    /// [ExtractFragment example]
}

TEST(ToPunycodeAscii, Empty) { EXPECT_EQ("", ToPunycodeAscii("")); }

TEST(ToPunycodeAscii, AsciiIsUnchanged) {
    EXPECT_EQ("example.com", ToPunycodeAscii("example.com"));
    EXPECT_EQ("my-site.example.com", ToPunycodeAscii("my-site.example.com"));
    // An already encoded domain is pure ASCII, so it is not encoded twice
    EXPECT_EQ("xn--bcher-kva.example.com", ToPunycodeAscii("xn--bcher-kva.example.com"));
}

TEST(ToPunycodeAscii, SingleLabel) {
    EXPECT_EQ("xn--bcher-kva", ToPunycodeAscii("bücher"));
    EXPECT_EQ("xn--mnchen-3ya", ToPunycodeAscii("münchen"));
    EXPECT_EQ("xn--tda", ToPunycodeAscii("ü"));
    EXPECT_EQ("xn--n3h", ToPunycodeAscii("☃"));
}

TEST(ToPunycodeAscii, NoBasicCodePoints) {
    // The whole label is non-ASCII, so there is no Punycode delimiter
    EXPECT_EQ("xn--p1ai", ToPunycodeAscii("рф"));
    EXPECT_EQ("xn--h1ahn", ToPunycodeAscii("мир"));
    EXPECT_EQ("xn--wgv71a119e", ToPunycodeAscii("日本語"));
    EXPECT_EQ("xn--hxajbheg2az3al", ToPunycodeAscii("παράδειγμα"));
}

TEST(ToPunycodeAscii, HyphensInBasicPart) {
    // The basic part may contain hyphens itself, only the last one is the delimiter
    EXPECT_EQ("xn--my-caf-gva", ToPunycodeAscii("my-café"));
    EXPECT_EQ("xn--a--yka", ToPunycodeAscii("a-ü"));
    EXPECT_EQ("xn---a-wka", ToPunycodeAscii("ü-a"));
}

TEST(ToPunycodeAscii, PerLabelEncoding) {
    EXPECT_EQ("xn--e1afmkfd.xn--p1ai", ToPunycodeAscii("пример.рф"));
    EXPECT_EQ("www.xn--e1afmkfd.xn--p1ai", ToPunycodeAscii("www.пример.рф"));
    // ASCII labels of a mixed domain are left as is
    EXPECT_EQ("shop.xn--80arbjktj.com", ToPunycodeAscii("shop.мойсайт.com"));
    EXPECT_EQ("xn--bcher-kva.example.com", ToPunycodeAscii("bücher.example.com"));
}

TEST(ToPunycodeAscii, EmptyLabels) {
    EXPECT_EQ("xn--p1ai.", ToPunycodeAscii("рф."));
    EXPECT_EQ(".xn--p1ai", ToPunycodeAscii(".рф"));
    EXPECT_EQ("xn--p1ai..com", ToPunycodeAscii("рф..com"));
}

TEST(ToPunycodeAscii, SupplementaryPlane) {
    // Code points above the BMP take 4 bytes in UTF-8
    EXPECT_EQ("xn--p61hqader3aj", ToPunycodeAscii("𝔘𝔫𝔦𝔠𝔬𝔡𝔢"));
}

TEST(ToPunycodeAscii, InvalidUtf8) {
    EXPECT_EQ(std::nullopt, ToPunycodeAscii("\xff\xfe"));
    // Truncated two byte sequence
    EXPECT_EQ(std::nullopt, ToPunycodeAscii("\xd1"));
    // Lone continuation byte
    EXPECT_EQ(std::nullopt, ToPunycodeAscii("a\x80z"));
    // Overlong encoding of '/'
    EXPECT_EQ(std::nullopt, ToPunycodeAscii("\xc0\xaf"));
    // Surrogate half
    EXPECT_EQ(std::nullopt, ToPunycodeAscii("\xed\xa0\x80"));
    // A single broken label invalidates the whole domain
    EXPECT_EQ(std::nullopt, ToPunycodeAscii("example.\xff.com"));
}

USERVER_NAMESPACE_END
