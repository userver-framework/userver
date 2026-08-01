#include <userver/http/url.hpp>

#include <array>
#include <cstdint>
#include <limits>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>
#include <utils/impl/internal_tag.hpp>

#include <fmt/args.h>
#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace http {

namespace {

constexpr std::string_view kSchemaSeparator = "://";
constexpr char kQuerySeparator = '?';
constexpr char kFragmentSeparator = '#';

// RFC 3986 unreserved characters plus some additional characters that are safe
// in path segments for S3 and similar APIs
bool IsPathSafeChar(unsigned char c) {
    // Alphanumeric characters are always safe
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return true;
    }
    // RFC 3986 unreserved: - _ . ~
    // Additional path-safe characters for S3 compatibility: $ & , : = @
    switch (c) {
        case '-':
        case '_':
        case '.':
        case '~':
        case '$':
        case '&':
        case ',':
        case ':':
        case '=':
        case '@':
            return true;
        default:
            return false;
    }
}

void EncodeByte(unsigned char symbol, std::string& result) {
    std::array<char, 3> bytes = {'%', 0, 0};
    bytes[1] = (symbol & 0xF0) / 16;
    bytes[1] += (bytes[1] > 9) ? 'A' - 10 : '0';
    bytes[2] = symbol & 0x0F;
    bytes[2] += (bytes[2] > 9) ? 'A' - 10 : '0';
    result.append(bytes.data(), bytes.size());
}

void UrlEncodeTo(std::string_view input_string, std::string& result) {
    for (const char symbol : input_string) {
        if (isalnum(symbol)) {
            result.append(1, symbol);
            continue;
        }
        switch (symbol) {
            case '-':
            case '_':
            case '.':
            case '!':
            case '~':
            case '*':
            case '(':
            case ')':
            case '\'':
                result.append(1, symbol);
                break;
            default:
                EncodeByte(static_cast<unsigned char>(symbol), result);
                break;
        }
    }
}

void UrlEncodePathSegmentTo(std::string_view input_string, std::string& result) {
    for (const unsigned char symbol : input_string) {
        if (IsPathSafeChar(symbol)) {
            result.append(1, symbol);
        } else {
            EncodeByte(symbol, result);
        }
    }
}

}  // namespace

namespace {

//  Punycode related functions. This code is based on go's one
// Copyright 2012 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Punycode parameters, RFC 3492 section 5. All the computations are done on std::uint32_t with
// explicit overflow checks, just like in the reference implementation from the RFC appendix.
constexpr std::uint32_t kPunycodeBase = 36;
constexpr std::uint32_t kPunycodeTMin = 1;
constexpr std::uint32_t kPunycodeTMax = 26;
constexpr std::uint32_t kPunycodeSkew = 38;
constexpr std::uint32_t kPunycodeDamp = 700;
constexpr std::uint32_t kPunycodeInitialBias = 72;
// The first non-basic code point: everything below it is copied to the output as is
constexpr std::uint32_t kPunycodeInitialN = 128;
constexpr std::uint32_t kPunycodeMaxInt = std::numeric_limits<std::uint32_t>::max();

// ASCII Compatible Encoding prefix, RFC 5890 section 2.3.2.5
constexpr std::string_view kAcePrefix = "xn--";

// Digit values 0..25 are 'a'..'z', 26..35 are '0'..'9'
char EncodePunycodeDigit(std::uint32_t digit) {
    UASSERT(digit < kPunycodeBase);
    return static_cast<char>(digit < 26 ? digit + 'a' : digit - 26 + '0');
}

// Bias adaptation, RFC 3492 section 6.1
std::uint32_t AdaptPunycodeBias(std::uint32_t delta, std::uint32_t num_points, bool first_time) {
    delta = first_time ? delta / kPunycodeDamp : delta / 2;
    delta += delta / num_points;

    std::uint32_t k = 0;
    while (delta > ((kPunycodeBase - kPunycodeTMin) * kPunycodeTMax) / 2) {
        delta /= kPunycodeBase - kPunycodeTMin;
        k += kPunycodeBase;
    }

    return k + (kPunycodeBase - kPunycodeTMin + 1) * delta / (delta + kPunycodeSkew);
}

// Punycode operates on code points rather than on bytes, so the whole label has to be decoded
// upfront. Returns std::nullopt if `text` is not valid UTF-8.
std::optional<std::vector<char32_t>> SplitIntoCodePoints(std::string_view text) {
    std::vector<char32_t> code_points;
    code_points.reserve(text.size());

    for (std::size_t pos = 0; pos < text.size();) {
        const auto length = utils::text::utf8::CodePointLengthByFirstByte(text[pos]);
        const auto sequence = text.substr(pos, length);
        // Rejects truncated sequences, overlong encodings, surrogates and out of range values
        if (!utils::text::utf8::IsWellFormedCodePoint(sequence)) {
            return std::nullopt;
        }

        // The leading byte carries 7, 5, 4 or 3 payload bits, every continuation byte carries 6
        const int leading_mask = (length == 1) ? 0x7F : (0x7F >> length);
        auto code_point = static_cast<char32_t>(static_cast<unsigned char>(sequence[0]) & leading_mask);
        for (std::size_t i = 1; i < length; ++i) {
            code_point = (code_point << 6) | (static_cast<unsigned char>(sequence[i]) & 0x3F);
        }

        code_points.push_back(code_point);
        pos += length;
    }

    return code_points;
}

// Punycode encoding of a single label, RFC 3492 section 6.3
std::optional<std::string> EncodePunycodeLabel(std::string_view label) {
    const auto code_points = SplitIntoCodePoints(label);
    if (!code_points) {
        return std::nullopt;
    }

    std::string result{kAcePrefix};

    // Basic code points are copied verbatim and are followed by the delimiter
    std::uint32_t basic_count = 0;
    for (const char32_t code_point : *code_points) {
        if (code_point < kPunycodeInitialN) {
            result.push_back(static_cast<char>(code_point));
            ++basic_count;
        }
    }
    if (basic_count > 0) {
        result.push_back('-');
    }

    std::uint32_t n = kPunycodeInitialN;
    std::uint32_t delta = 0;
    std::uint32_t bias = kPunycodeInitialBias;
    std::uint32_t handled = basic_count;

    while (handled < code_points->size()) {
        // The smallest code point that has not been handled yet
        std::uint32_t next_n = kPunycodeMaxInt;
        for (const char32_t code_point : *code_points) {
            if (code_point >= n && code_point < next_n) {
                next_n = code_point;
            }
        }

        if (next_n - n > (kPunycodeMaxInt - delta) / (handled + 1)) {
            return std::nullopt;
        }
        delta += (next_n - n) * (handled + 1);
        n = next_n;

        for (const char32_t code_point : *code_points) {
            if (code_point < n) {
                if (++delta == 0) {
                    return std::nullopt;
                }
                continue;
            }
            if (code_point > n) {
                continue;
            }

            // Represent delta as a generalized variable-length integer
            std::uint32_t q = delta;
            for (std::uint32_t k = kPunycodeBase;; k += kPunycodeBase) {
                const std::uint32_t t = (k <= bias + kPunycodeTMin)   ? kPunycodeTMin
                                        : (k >= bias + kPunycodeTMax) ? kPunycodeTMax
                                                                      : k - bias;
                if (q < t) {
                    break;
                }
                result.push_back(EncodePunycodeDigit(t + (q - t) % (kPunycodeBase - t)));
                q = (q - t) / (kPunycodeBase - t);
            }
            result.push_back(EncodePunycodeDigit(q));

            bias = AdaptPunycodeBias(delta, handled + 1, handled == basic_count);
            delta = 0;
            ++handled;
        }

        ++delta;
        ++n;
    }

    return result;
}

}  // namespace

std::optional<std::string> ToPunycodeAscii(std::string_view domain) {
    if (utils::text::IsAscii(domain)) {
        return std::string{domain};
    }

    std::string result;
    result.reserve(domain.size() + kAcePrefix.size());

    // Every label is encoded on its own, the ACE prefix is per label as well
    std::size_t label_begin = 0;
    while (true) {
        const auto dot_pos = domain.find('.', label_begin);
        const auto label = domain.substr(label_begin, dot_pos - label_begin);

        if (utils::text::IsAscii(label)) {
            result.append(label);
        } else {
            const auto encoded_label = EncodePunycodeLabel(label);
            if (!encoded_label) {
                return std::nullopt;
            }
            result.append(*encoded_label);
        }

        if (dot_pos == std::string_view::npos) {
            break;
        }
        result.push_back('.');
        label_begin = dot_pos + 1;
    }

    return result;
}

std::string UrlEncode(std::string_view input_string) {
    std::string result;
    result.reserve(3 * input_string.size());

    UrlEncodeTo(input_string, result);
    return result;
}

std::string UrlEncodePathSegment(std::string_view input_string) {
    std::string result;
    result.reserve(3 * input_string.size());

    UrlEncodePathSegmentTo(input_string, result);
    return result;
}

std::string EncodeS3Key(std::string_view key) {
    std::string result;
    result.reserve(key.size());

    size_t start = 0;
    while (start < key.size()) {
        size_t slash_pos = key.find('/', start);
        if (slash_pos == std::string::npos) {
            UrlEncodePathSegmentTo(key.substr(start), result);
            break;
        } else {
            UrlEncodePathSegmentTo(key.substr(start, slash_pos - start), result);
            result.push_back('/');
            start = slash_pos + 1;
        }
    }

    return result;
}

std::string UrlDecode(std::string_view range) { return impl::UrlDecode(utils::impl::InternalTag{}, range); }

namespace {

template <typename T>
std::size_t GetInitialQueryCapacity(T begin, T end) {
    std::size_t capacity = 1;
    for (auto it = begin; it != end; ++it) {
        // Maximal query result size is 3 * input.size. Coefficient 3 / 2 guarantee
        // no more than one reallocation.
        capacity += 1 + (it->first.size() + it->second.size()) * 3 / 2 + 1;
    }
    return capacity;
}

template <typename T>
void DoMakeQueryTo(T begin, T end, std::string& result) {
    bool first = true;
    for (auto it = begin; it != end; ++it) {
        if (!first) {
            result.append(1, '&');
        } else {
            first = false;
        }
        UrlEncodeTo(it->first, result);
        result.append(1, '=');
        UrlEncodeTo(it->second, result);
    }
}

template <typename T>
std::string DoMakeQuery(T begin, T end) {
    std::string result;
    result.reserve(GetInitialQueryCapacity(begin, end));

    DoMakeQueryTo(begin, end, result);
    return result;
}

template <typename T>
std::string MakeUrl(std::string_view path, T begin, T end) {
    if (begin == end) {
        return std::string{path};
    }

    std::string result;
    result.reserve(path.size() + 1 + GetInitialQueryCapacity(begin, end));

    result.append(path);
    result.append(1, '?');
    DoMakeQueryTo(begin, end, result);
    return result;
}

template <typename T>
std::optional<std::string> MakeUrlWithPathArgsImpl(std::string_view path, const PathArgs& path_args, T begin, T end) {
    auto built_path_opt = MakeUrlWithPathArgs(path, path_args);
    if (!built_path_opt.has_value()) {
        return std::nullopt;
    }

    if (begin == end) {
        return built_path_opt;
    }

    auto built_path = std::move(built_path_opt).value();
    built_path.reserve(built_path.size() + GetInitialQueryCapacity(begin, end));

    built_path.append(1, '?');
    DoMakeQueryTo(begin, end, built_path);

    return built_path;
}

}  // namespace

std::string MakeUrl(std::string_view path, const Args& query_args) {
    return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeUrl(std::string_view path, const std::unordered_map<std::string, std::string>& query_args) {
    return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeUrl(std::string_view path, const Args& query_args, MultiArgs query_multiargs) {
    for (const auto& [key, value] : query_args) {
        query_multiargs.insert({key, value});
    }
    return MakeUrl(path, query_multiargs.begin(), query_multiargs.end());
}

std::string MakeUrl(
    std::string_view path,
    std::initializer_list<std::pair<std::string_view, std::string_view>> query_args
) {
    return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeUrl(
    std::string_view path,
    const std::vector<std::pair<std::string_view, std::string_view>>& query_args
) {
    return MakeUrl(path, query_args.begin(), query_args.end());
}

std::string MakeQuery(const Args& query_args) { return DoMakeQuery(query_args.begin(), query_args.end()); }

std::string MakeQuery(const MultiArgs& query_args) { return DoMakeQuery(query_args.begin(), query_args.end()); }

std::string MakeQuery(const std::unordered_map<std::string, std::string>& query_args) {
    return DoMakeQuery(query_args.begin(), query_args.end());
}

std::string MakeQuery(std::initializer_list<std::pair<std::string_view, std::string_view>> query_args) {
    return DoMakeQuery(query_args.begin(), query_args.end());
}

std::optional<std::string> MakeUrlWithPathArgs(std::string_view path_template, const PathArgs& path_args) {
    if (path_args.empty()) {
        return std::string{path_template};
    }

    fmt::dynamic_format_arg_store<fmt::format_context> fmt_args;
    fmt_args.reserve(path_args.size(), 0);

    for (const auto& [key, value] : path_args) {
        if (key.empty()) {
            return std::nullopt;
        }
        fmt_args.push_back(fmt::arg(key.c_str(), UrlEncode(value)));
    }

    try {
        return fmt::vformat(path_template, fmt_args);
    } catch (const fmt::format_error& exc) {
        LOG_ERROR() << "Failed to format URL path template: '" << path_template << "'. Format error: " << exc.what();
        return std::nullopt;
    }
}

std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    const Args& query_args
) {
    return MakeUrlWithPathArgsImpl(path, path_args, query_args.begin(), query_args.end());
}

std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    const std::unordered_map<std::string, std::string>& query_args
) {
    return MakeUrlWithPathArgsImpl(path, path_args, query_args.begin(), query_args.end());
}

std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    const Args& query_args,
    MultiArgs query_multiargs
) {
    for (const auto& [key, value] : query_args) {
        query_multiargs.insert({key, value});
    }
    return MakeUrlWithPathArgsImpl(path, path_args, query_multiargs.begin(), query_multiargs.end());
}

std::optional<std::string> MakeUrlWithPathArgs(
    std::string_view path,
    const PathArgs& path_args,
    std::initializer_list<std::pair<std::string_view, std::string_view>> query_args
) {
    return MakeUrlWithPathArgsImpl(path, path_args, query_args.begin(), query_args.end());
}

std::string_view ExtractMetaTypeFromUrlView(std::string_view url) {
    auto pos = url.find(kQuerySeparator);
    if (pos == std::string::npos) {
        return url;
    }

    return url.substr(0, pos);
}

std::string ExtractMetaTypeFromUrl(std::string_view url) { return std::string{ExtractMetaTypeFromUrlView(url)}; }

std::string_view ExtractPathView(std::string_view url) {
    const auto pos = url.find(kSchemaSeparator);
    // Cut scheme
    auto tmp = (pos == std::string::npos) ? url : url.substr(pos + kSchemaSeparator.size());

    const auto slash_pos = tmp.find('/');
    if (slash_pos == std::string::npos) {
        return {};
    }
    tmp = tmp.substr(slash_pos);

    auto query_pos = tmp.find(kQuerySeparator);
    const auto fragment_pos = tmp.find(kFragmentSeparator);

    // Handle case when fragment placed right after path
    if (fragment_pos != std::string::npos) {
        tmp = tmp.substr(0, fragment_pos);
        query_pos = tmp.find(kQuerySeparator);
    }

    if (query_pos != std::string::npos) {
        tmp = tmp.substr(0, query_pos);
    }
    return tmp;
}

std::string ExtractPath(std::string_view url) { return std::string{ExtractPathView(url)}; }

std::string ExtractPathOnly(std::string_view url) {
    auto path = ExtractPathView(url);
    auto pos = path.find('?');
    if (pos != std::string::npos) {
        return std::string{path.substr(0, pos)};
    } else {
        return std::string{path};
    }
}

std::string_view ExtractHostnameView(std::string_view url) {
    // Drop "schema://"
    const auto pos = url.find(kSchemaSeparator);
    auto tmp = (pos == std::string::npos) ? url : url.substr(pos + kSchemaSeparator.size());

    // Drop /.*
    const auto slash_pos = tmp.find('/');
    if (slash_pos != std::string::npos) {
        tmp = tmp.substr(0, slash_pos);
    }

    const auto userinfo_pos = tmp.rfind('@');
    if (userinfo_pos != std::string::npos) {
        tmp = tmp.substr(userinfo_pos + 1);
    }

    const auto bracket_close_pos = tmp.find(']');
    if (bracket_close_pos != std::string::npos) {
        // IPv6 address
        tmp = tmp.substr(0, bracket_close_pos + 1);
    } else {
        // DNS name or IPv4 address
        const auto port_pos = tmp.find(':');
        if (port_pos != std::string::npos) {
            tmp = tmp.substr(0, port_pos);
        }
    }

    return tmp;
}

std::string ExtractHostname(std::string_view url) { return std::string{ExtractHostnameView(url)}; }

std::string_view ExtractSchemeView(std::string_view url) {
    const auto pos = url.find(kSchemaSeparator);
    return (pos == std::string::npos) ? "" : url.substr(0, pos);
}

std::string ExtractScheme(std::string_view url) { return std::string{ExtractSchemeView(url)}; }

std::string_view ExtractQueryView(std::string_view url) {
    auto query_start = url.find(kQuerySeparator);
    if (query_start == std::string::npos || query_start + 1 >= url.size()) {
        return {};
    }

    ++query_start;

    const auto fragment_start = url.find(kFragmentSeparator);
    const auto query_end = (fragment_start == std::string::npos) ? url.size() : fragment_start;

    return url.substr(query_start, query_end - query_start);
}

std::string ExtractQuery(std::string_view url) { return std::string{ExtractQueryView(url)}; }

std::string_view ExtractFragmentView(std::string_view url) {
    const auto pos = url.find(kFragmentSeparator);
    return (pos == std::string::npos) || pos + 1 >= url.size() ? "" : url.substr(pos + 1);
}

std::string ExtractFragment(std::string_view url) { return std::string{ExtractFragmentView(url)}; }

DecomposedUrlView DecomposeUrlIntoViews(std::string_view url) {
    DecomposedUrlView result;
    result.scheme = ExtractSchemeView(url);
    result.host = ExtractHostnameView(url);
    result.path = ExtractPathView(url);
    result.query = ExtractQueryView(url);
    result.fragment = ExtractFragmentView(url);
    return result;
}

namespace impl {

std::string UrlDecode(utils::impl::InternalTag, std::string_view range) {
    std::string result;
    result.reserve(range.size() / 3);

    for (const char *i = range.begin(), *end = range.end(); i != end; ++i) {
        switch (*i) {
            case '+':
                result.append(1, ' ');
                break;
            case '%':
                if (std::distance(i, end) > 2) {
                    const char f = *(i + 1);
                    const char s = *(i + 2);
                    int digit = (f >= 'A' ? ((f & 0xDF) - 'A') + 10 : (f - '0')) * 16;
                    digit += (s >= 'A') ? ((s & 0xDF) - 'A') + 10 : (s - '0');
                    result.append(1, static_cast<char>(digit));
                    i += 2;
                } else {
                    result.append(1, '%');
                }
                break;
            default:
                result.append(1, (*i));
                break;
        }
    }

    return result;
}

}  // namespace impl

}  // namespace http

USERVER_NAMESPACE_END
