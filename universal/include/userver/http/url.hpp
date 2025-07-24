#pragma once

/// @file userver/http/url.hpp
/// @brief URL manipulation functions
/// @ingroup userver_universal

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/utils/impl/internal_tag_fwd.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace http {

struct DecomposedUrlView {
    std::string_view scheme;
    std::string_view host;
    std::string_view path;
    std::string_view query;
    std::string_view fragment;
};

/// @brief Decode URL
[[deprecated("Use a more strict http::parser::UrlDecode instead")]] std::string UrlDecode(std::string_view range);

/// @brief Encode as URL
/// @param input_string String to encode
/// @returns URL-encoded string where special characters are encoded as %XX sequences
/// @code
///   auto encoded = UrlEncode("hello world");
///   // Returns: "hello%20world"
/// @endcode
std::string UrlEncode(std::string_view input_string);

using Args = std::unordered_map<std::string, std::string, utils::StrCaseHash>;
using MultiArgs = std::multimap<std::string, std::string>;
using PathArgs = std::unordered_map<std::string, std::string>;

/// @brief Make an URL query
/// @param query_args Map of query parameters
/// @returns URL query string without leading '?' character
/// @code
///   auto query = MakeQuery(http::Args{{"param", "value"}, {"filter", "active"}});
///   // Returns: "param=value&filter=active"
/// @endcode
std::string MakeQuery(const Args& query_args);

/// @brief Make an URL query
/// @param query_args Multimap of query parameters
/// @returns URL query string without leading '?' character
/// @code
///   http::MultiArgs args = {{"tag", "new"}, {"tag", "featured"}};
///   auto query = MakeQuery(args);
///   // Returns: "tag=new&tag=featured"
/// @endcode
std::string MakeQuery(const MultiArgs& query_args);

/// @brief Make an URL query
/// @param query_args Map of query parameters
/// @returns URL query string without leading '?' character
/// @code
///   auto query = MakeQuery(std::unordered_map<std::string, std::string>{{"page", "1"}, {"size", "10"}});
///   // Returns: "page=1&size=10"
/// @endcode
std::string MakeQuery(const std::unordered_map<std::string, std::string>& query_args);

/// @brief Make an URL query
/// @param query_args Initializer list of query parameters as key-value pairs
/// @returns URL query string without leading '?' character
/// @code
///   auto query = MakeQuery({{"sort", "date"}, {"order", "desc"}});
///   // Returns: "sort=date&order=desc"
/// @endcode
std::string MakeQuery(std::initializer_list<std::pair<std::string_view, std::string_view>> query_args);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args Map of query parameters
/// @returns Complete URL with query string
/// @code
///   auto url = MakeUrl("/api/users", http::Args{{"status", "active"}});
///   // Returns: "/api/users?status=active"
/// @endcode
std::string MakeUrl(std::string_view path, const Args& query_args);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args Map of query parameters
/// @returns Complete URL with query string
/// @code
///   auto url = MakeUrl("/api/products", std::unordered_map<std::string, std::string>{{"category", "electronics"}});
///   // Returns: "/api/products?category=electronics"
/// @endcode
std::string MakeUrl(std::string_view path, const std::unordered_map<std::string, std::string>& query_args);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args Map of query parameters
/// @param query_multiargs Multimap for query parameters that can have multiple values
/// @returns Complete URL with query string
/// @code
///   http::MultiArgs multi_args = {{"tag", "new"}, {"tag", "featured"}};
///   auto url = MakeUrl("/api/products", http::Args{{"category", "electronics"}}, multi_args);
///   // Returns: "/api/products?category=electronics&tag=new&tag=featured"
/// @endcode
std::string MakeUrl(std::string_view path, const Args& query_args, MultiArgs query_multiargs);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args Initializer list of query parameters as key-value pairs
/// @returns Complete URL with query string
/// @code
///   auto url = MakeUrl("/api/search", {{"q", "smartphone"}, {"sort", "relevance"}});
///   // Returns: "/api/search?q=smartphone&sort=relevance"
/// @endcode
std::string
MakeUrl(std::string_view path, std::initializer_list<std::pair<std::string_view, std::string_view>> query_args);

/// @brief Make a path from a template and arguments
/// @param path Template string with placeholders in format {name}
/// @param path_args Map of placeholder names to their values
/// @returns Formatted path or std::nullopt if formatting fails (e.g., missing placeholder,
///          invalid format, or empty key in path_args)
/// @code
///   auto url = MakeUrlWithPathArgs("/api/v1/users/{user_id}", {{"user_id", "123"}});
///   // Returns: "/api/v1/users/123"
/// @endcode
std::optional<std::string> MakeUrlWithPathArgs(std::string_view path, const PathArgs& path_args);

/// @brief Make an URL with path parameters and query arguments
/// @param path Template string with placeholders in format {name}
/// @param path_args Map of placeholder names to their values
/// @param query_args Map of query parameters
/// @returns Formatted URL or std::nullopt if path formatting fails
/// @code
///   auto url = MakeUrlWithPathArgs("/api/v1/users/{user_id}",
///                                 {{"user_id", "123"}},
///                                 http::Args{{"filter", "active"}});
///   // Returns: "/api/v1/users/123?filter=active"
/// @endcode
std::optional<std::string>
MakeUrlWithPathArgs(std::string_view path, const PathArgs& path_args, const Args& query_args);

/// @brief Make an URL with path parameters and query arguments
/// @param path Template string with placeholders in format {name}
/// @param path_args Map of placeholder names to their values
/// @param query_args Map of query parameters
/// @returns Formatted URL or std::nullopt if path formatting fails
/// @code
///   auto url = MakeUrlWithPathArgs("/api/v1/users/{user_id}",
///                                 {{"user_id", "123"}},
///                                 std::unordered_map<std::string, std::string>{{"page", "1"}});
///   // Returns: "/api/v1/users/123?page=1"
/// @endcode
std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    const std::unordered_map<std::string, std::string>& query_args
);

/// @brief Make an URL with path parameters and query arguments, supporting multiple values for the same key
/// @param path Template string with placeholders in format {name}
/// @param path_args Map of placeholder names to their values
/// @param query_args Map of query parameters
/// @param query_multiargs Multimap for query parameters that can have multiple values
/// @returns Formatted URL or std::nullopt if path formatting fails
/// @code
///   http::MultiArgs multi_args = {{"tag", "new"}, {"tag", "featured"}};
///   auto url = MakeUrlWithPathArgs("/api/v1/products/{category}",
///                                 {{"category", "electronics"}},
///                                 http::Args{{"sort", "price"}},
///                                 multi_args);
///   // Returns: "/api/v1/products/electronics?sort=price&tag=new&tag=featured"
/// @endcode
std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    const Args& query_args,
    MultiArgs query_multiargs
);

/// @brief Make an URL with path parameters and query arguments
/// @param path Template string with placeholders in format {name}
/// @param path_args Map of placeholder names to their values
/// @param query_args Initializer list of query parameters as key-value pairs
/// @returns Formatted URL or std::nullopt if path formatting fails
/// @code
///   auto url = MakeUrlWithPathArgs("/api/v1/search/{term}",
///                                 {{"term", "laptop"}},
///                                 {{"brand", "apple"}, {"price_max", "2000"}});
///   // Returns: "/api/v1/search/laptop?brand=apple&price_max=2000"
/// @endcode
std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    std::initializer_list<std::pair<std::string_view, std::string_view>> query_args
);

/// @brief Returns URL part before the first '?' character
/// @param url Full URL to extract from
/// @returns URL without query string
/// @code
///   auto base = ExtractMetaTypeFromUrl("https://example.com/api/users?page=1&sort=name");
///   // Returns: "https://example.com/api/users"
/// @endcode
std::string ExtractMetaTypeFromUrl(std::string_view url);
std::string_view ExtractMetaTypeFromUrlView(std::string_view url);

// TODO: rename to ExtractPathAndQuery()
/// @brief Returns HTTP path part of a URL
/// @param url Full URL to extract from
/// @returns Path component of the URL
/// @code
///   auto path = ExtractPath("https://example.com/api/users");
///   // Returns: "/api/users"
///   auto path2 = ExtractPath("example.com/api/users?a=b");
///   // Returns: "/api/users?a=b"
/// @endcode
std::string ExtractPath(std::string_view url);
std::string_view ExtractPathView(std::string_view url);

/// @brief Returns HTTP path part of a URL
/// @param url Full URL to extract from
/// @returns Path component of the URL
/// @code
///   auto path = ExtractPath("https://example.com/api/users");
///   // Returns: "/api/users"
///   auto path2 = ExtractPath("example.com/api/users?a=b");
///   // Returns: "/api/users"
/// @endcode
std::string ExtractPathOnly(std::string_view url);

/// @brief Returns hostname part of a URL
/// @param url Full URL to extract from
/// @returns Hostname component of the URL
/// @code
///   auto host = ExtractHostname("https://example.com/api/users");
///   // Returns: "example.com"
///   auto host2 = ExtractHostname("https://user:pass@example.com:8080/api");
///   // Returns: "example.com"
///   auto host3 = ExtractHostname("http://[::1]:8080/");
///   // Returns: "[::1]"
/// @endcode
std::string ExtractHostname(std::string_view url);
std::string_view ExtractHostnameView(std::string_view url);

/// @brief Returns scheme part of a URL
/// @param url Full URL to extract from
/// @returns Scheme component of the URL
/// @code
///   auto scheme = ExtractScheme("https://example.com/api/users");
///   // Returns: "https"
///   auto scheme2 = ExtractScheme("http://user:pass@example.com:8080/api");
///   // Returns: "http"
///   auto scheme3 = ExtractScheme("ftp://[::1]:8080/");
///   // Returns: "ftp"
/// @endcode
std::string ExtractScheme(std::string_view url);
std::string_view ExtractSchemeView(std::string_view url);

/// @brief Returns query part of a URL
/// @param url Full URL to extract from
/// @returns Query component of the URL
/// @code
///   auto query = ExtractQuery("https://example.com/api/users?q=1");
///   // Returns: "q=1"
///   auto query2 = ExtractQuery("http://user:pass@example.com:8080/api");
///   // Returns: ""
///   auto query3 = ExtractQuery("ftp://[::1]:8080/?q=12&w=23");
///   // Returns: "q=12&w=23"
/// @endcode
std::string ExtractQuery(std::string_view url);
std::string_view ExtractQueryView(std::string_view url);

/// @brief Returns fragment part of a URL
/// @param url Full URL to extract from
/// @returns Fragment component of the URL
/// @code
///   auto fragment = ExtractFragment("https://example.com/api/users?q=1");
///   // Returns: ""
///   auto fragment2 = ExtractFragment("http://user:pass@example.com:8080/api#123");
///   // Returns: "123"
///   auto fragment3 = ExtractFragment("ftp://[::1]:8080/#123?q=12&w=23");
///   // Returns: "123"
///   auto fragment4 = ExtractFragment("ftp://[::1]:8080/?q=12&w=23#123");
///   // Returns: "123"
/// @endcode
std::string ExtractFragment(std::string_view url);
std::string_view ExtractFragmentView(std::string_view url);

/// @brief Returns decomposed URL as a struct,
/// broken into main parts: scheme, host, path, query, and fragment
DecomposedUrlView DecomposeUrlIntoViews(std::string_view url);

namespace impl {

std::string UrlDecode(utils::impl::InternalTag, std::string_view range);

}  // namespace impl

}  // namespace http

USERVER_NAMESPACE_END
