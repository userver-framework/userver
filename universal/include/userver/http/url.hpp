#pragma once

/// @file userver/http/url.hpp
/// @brief URL manipulation functions
/// @ingroup userver_universal

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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
/// @snippet universal/src/http/url_test.cpp  UrlEncode example
std::string UrlEncode(std::string_view input_string);

/// @brief Encode a URL path segment (for use in S3 and similar APIs)
/// @param input_string String to encode
/// @returns URL-encoded string where special characters are encoded as %XX sequences,
///          but path-safe characters (-, _, ., ~, $, &, ,, :, =, @) are kept unescaped
/// @note This is less aggressive than UrlEncode and is suitable for encoding path segments
///       where you want to preserve readability of certain special characters
/// @snippet universal/src/http/url_test.cpp  UrlEncodePathSegment example
std::string UrlEncodePathSegment(std::string_view input_string);

/// @brief Encode an S3 object key for use in URL path
/// @param key S3 object key (may contain '/' as part of the key name)
/// @returns URL-encoded path where each segment is encoded but '/' separators are preserved
/// @note S3 object keys can contain '/' which should be preserved as path separators,
///       while other special characters should be encoded
/// @snippet universal/src/http/url_test.cpp  EncodeS3Key example
std::string EncodeS3Key(std::string_view key);

using Args = std::unordered_map<std::string, std::string, utils::StrCaseHash>;
using MultiArgs = std::multimap<std::string, std::string>;
using PathArgs = std::unordered_map<std::string, std::string>;

/// @brief Make an URL query
/// @param query_args Map of query parameters
/// @returns URL query string without leading '?' character
/// @snippet universal/src/http/url_test.cpp  MakeQuery example
std::string MakeQuery(const Args& query_args);

/// @brief Make an URL query
/// @param query_args Multimap of query parameters
/// @returns URL query string without leading '?' character
/// @snippet universal/src/http/url_test.cpp  MakeQuery MultiArgs
std::string MakeQuery(const MultiArgs& query_args);

/// @brief Make an URL query
/// @param query_args Map of query parameters
/// @returns URL query string without leading '?' character
/// @snippet universal/src/http/url_test.cpp  MakeQuery unordered_map example
std::string MakeQuery(const std::unordered_map<std::string, std::string>& query_args);

/// @brief Make an URL query
/// @param query_args Initializer list of query parameters as key-value pairs
/// @returns URL query string without leading '?' character
/// @snippet universal/src/http/url_test.cpp  MakeQuery initializer list example
std::string MakeQuery(std::initializer_list<std::pair<std::string_view, std::string_view>> query_args);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args Map of query parameters
/// @returns Complete URL with query string
/// @snippet universal/src/http/url_test.cpp  MakeUrl example
std::string MakeUrl(std::string_view path, const Args& query_args);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args Map of query parameters
/// @returns Complete URL with query string
/// @snippet universal/src/http/url_test.cpp  MakeUrl with unordered_map example
std::string MakeUrl(std::string_view path, const std::unordered_map<std::string, std::string>& query_args);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args Map of query parameters
/// @param query_multiargs Multimap for query parameters that can have multiple values
/// @returns Complete URL with query string
/// @snippet universal/src/http/url_test.cpp  MakeUrl with MultiArgs example
std::string MakeUrl(std::string_view path, const Args& query_args, MultiArgs query_multiargs);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args Initializer list of query parameters as key-value pairs
/// @returns Complete URL with query string
/// @snippet universal/src/http/url_test.cpp  MakeUrl with initializer list example
std::string MakeUrl(
    std::string_view path,
    std::initializer_list<std::pair<std::string_view, std::string_view>> query_args
);

/// @brief Make an URL with query arguments
/// @param path Base URL path
/// @param query_args vector of query parameters as key-value pairs
/// @returns Complete URL with query string
std::string MakeUrl(
    std::string_view path,
    const std::vector<std::pair<std::string_view, std::string_view>>& query_args
);

/// @brief Make a path from a template and arguments
/// @param path Template string with placeholders in format {name}
/// @param path_args Map of placeholder names to their values
/// @returns Formatted path or std::nullopt if formatting fails (e.g., missing placeholder,
///          invalid format, or empty key in path_args)
/// @snippet universal/src/http/url_test.cpp  MakeUrlWithPathArgs example
std::optional<std::string> MakeUrlWithPathArgs(std::string_view path, const PathArgs& path_args);

/// @brief Make an URL with path parameters and query arguments
/// @param path Template string with placeholders in format {name}
/// @param path_args Map of placeholder names to their values
/// @param query_args Map of query parameters
/// @returns Formatted URL or std::nullopt if path formatting fails
/// @snippet universal/src/http/url_test.cpp  MakeUrlWithPathArgs with query example
std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    const Args& query_args
);

/// @brief Make an URL with path parameters and query arguments
/// @param path Template string with placeholders in format {name}
/// @param path_args Map of placeholder names to their values
/// @param query_args Map of query parameters
/// @returns Formatted URL or std::nullopt if path formatting fails
/// @snippet universal/src/http/url_test.cpp  MakeUrlWithPathArgs unordered map query args
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
/// @snippet universal/src/http/url_test.cpp  MakeUrlWithPathArgs MultiArgs
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
/// @snippet universal/src/http/url_test.cpp  MakeUrlWithPathArgs with initializer list example
std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    std::initializer_list<std::pair<std::string_view, std::string_view>> query_args
);

/// @brief Returns URL part before the first '?' character
/// @param url Full URL to extract from
/// @returns URL without query string
/// @snippet universal/src/http/url_test.cpp  ExtractMetaTypeFromUrl example
std::string ExtractMetaTypeFromUrl(std::string_view url);
std::string_view ExtractMetaTypeFromUrlView(std::string_view url);

// TODO: rename to ExtractPathAndQuery()
/// @brief Returns HTTP path part of a URL
/// @param url Full URL to extract from
/// @returns Path component of the URL
/// @snippet universal/src/http/url_test.cpp  ExtractPathOnly example
std::string ExtractPath(std::string_view url);
std::string_view ExtractPathView(std::string_view url);

/// @brief Returns HTTP path part of a URL
/// @param url Full URL to extract from
/// @returns Path component of the URL
/// @snippet universal/src/http/url_test.cpp  ExtractPathOnly example 2
std::string ExtractPathOnly(std::string_view url);

/// @brief Returns hostname part of a URL
/// @param url Full URL to extract from
/// @returns Hostname component of the URL
/// @snippet universal/src/http/url_test.cpp  ExtractHostname example
std::string ExtractHostname(std::string_view url);
std::string_view ExtractHostnameView(std::string_view url);

/// @brief Returns scheme part of a URL
/// @param url Full URL to extract from
/// @returns Scheme component of the URL
/// @snippet universal/src/http/url_test.cpp  ExtractScheme example
std::string ExtractScheme(std::string_view url);
std::string_view ExtractSchemeView(std::string_view url);

/// @brief Returns query part of a URL
/// @param url Full URL to extract from
/// @returns Query component of the URL
/// @snippet universal/src/http/url_test.cpp  ExtractQuery example
std::string ExtractQuery(std::string_view url);
std::string_view ExtractQueryView(std::string_view url);

/// @brief Returns fragment part of a URL
/// @param url Full URL to extract from
/// @returns Fragment component of the URL
/// @snippet universal/src/http/url_test.cpp  ExtractFragment example
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
