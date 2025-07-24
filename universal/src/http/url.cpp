#include <userver/http/url.hpp>

#include <array>

#include <userver/logging/log.hpp>
#include <utils/impl/internal_tag.hpp>

#include <fmt/args.h>
#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace http {

namespace {

constexpr std::string_view kSchemaSeparator = "://";
constexpr char kQuerySeparator = '?';
constexpr char kFragmentSeparator = '#';

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
                std::array<char, 3> bytes = {'%', 0, 0};
                bytes[1] = (symbol & 0xF0) / 16;
                bytes[1] += (bytes[1] > 9) ? 'A' - 10 : '0';
                bytes[2] = symbol & 0x0F;
                bytes[2] += (bytes[2] > 9) ? 'A' - 10 : '0';
                result.append(bytes.data(), bytes.size());
                break;
        }
    }
}

}  // namespace

std::string UrlEncode(std::string_view input_string) {
    std::string result;
    result.reserve(3 * input_string.size());

    UrlEncodeTo(input_string, result);
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

std::string
MakeUrl(std::string_view path, std::initializer_list<std::pair<std::string_view, std::string_view>> query_args) {
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
        fmt_args.push_back(fmt::arg(key.data(), UrlEncode(value)));
    }

    try {
        return fmt::vformat(path_template, fmt_args);
    } catch (const fmt::format_error& exc) {
        LOG_ERROR() << "Failed to format URL path template: '" << path_template << "'. Format error: " << exc.what();
        return std::nullopt;
    }
}

std::optional<std::string>
MakeUrlWithPathArgs(std::string_view path, const PathArgs& path_args, const Args& query_args) {
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
    if (pos == std::string::npos) return url;

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
