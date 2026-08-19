#include "dsn.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <unordered_map>
#include <vector>

#include <userver/clients/dns/resolver.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace {

constexpr std::string_view kServerKeys[] = {"SERVER", "SERVERNAME", "HOST", "HOSTNAME"};
constexpr std::string_view kPortKeys[] = {"PORT"};
constexpr std::string_view kDatabaseKeys[] = {"DATABASE", "DB", "DBQ"};
constexpr std::string_view kDriverKeys[] = {"DRIVER"};
constexpr std::string_view kUserKeys[] = {"UID", "USER", "USERNAME"};
constexpr std::string_view kPasswordKeys[] = {"PWD", "PASSWORD"};

using KeyValueMap = std::unordered_map<std::string, std::string>;

struct DsnPart final {
    std::string_view raw{};
    std::string key{};
    std::string value{};
    std::size_t value_begin{0};
    std::size_t value_end{0};
    bool valid{false};
};

std::string_view Trim(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.remove_suffix(1);
    }
    return value;
}

std::vector<std::string_view> SplitDsn(std::string_view dsn) {
    std::vector<std::string_view> result;
    std::size_t part_begin = 0;
    bool in_braces = false;
    bool saw_equal = false;
    bool saw_value = false;

    for (std::size_t index = 0; index < dsn.size(); ++index) {
        const auto c = dsn[index];
        if (!saw_equal && c == '=') {
            saw_equal = true;
            continue;
        }
        if (saw_equal && !saw_value && !std::isspace(static_cast<unsigned char>(c))) {
            saw_value = true;
            in_braces = c == '{';
            continue;
        }
        if (in_braces && c == '}') {
            if (index + 1 < dsn.size() && dsn[index + 1] == '}') {
                ++index;
            } else {
                in_braces = false;
            }
            continue;
        }
        if (!in_braces && c == ';') {
            result.push_back(dsn.substr(part_begin, index - part_begin + 1));
            part_begin = index + 1;
            saw_equal = false;
            saw_value = false;
        }
    }
    if (part_begin < dsn.size()) {
        result.push_back(dsn.substr(part_begin));
    }
    return result;
}

std::string DecodeValue(std::string_view raw_value) {
    const auto trimmed = Trim(raw_value);
    if (trimmed.size() >= 2 && trimmed.front() == '{' && trimmed.back() == '}') {
        std::string decoded;
        decoded.reserve(trimmed.size() - 2);
        for (std::size_t index = 1; index + 1 < trimmed.size(); ++index) {
            if (trimmed[index] == '}' && index + 1 < trimmed.size() - 1 && trimmed[index + 1] == '}') {
                ++index;
            }
            decoded += trimmed[index];
        }
        return decoded;
    }
    return std::string{raw_value};
}

DsnPart ParsePart(std::string_view raw) {
    const auto content_end = !raw.empty() && raw.back() == ';' ? raw.size() - 1 : raw.size();
    const auto equal = raw.substr(0, content_end).find('=');
    if (equal == std::string_view::npos) {
        return {.raw = raw};
    }

    const auto key = Trim(raw.substr(0, equal));
    if (key.empty()) {
        return {.raw = raw};
    }
    const auto value_begin = equal + 1;
    return {
        .raw = raw,
        .key = utils::text::ToUpper(std::string{key}),
        .value = DecodeValue(raw.substr(value_begin, content_end - value_begin)),
        .value_begin = value_begin,
        .value_end = content_end,
        .valid = true,
    };
}

std::vector<DsnPart> ParseDsnParts(std::string_view dsn) {
    std::vector<DsnPart> result;
    for (const auto raw : SplitDsn(dsn)) {
        result.push_back(ParsePart(raw));
    }
    return result;
}

KeyValueMap ParseDsnToMap(std::string_view dsn) {
    KeyValueMap result;
    for (auto& part : ParseDsnParts(dsn)) {
        if (part.valid) {
            result[std::move(part.key)] = std::move(part.value);
        }
    }
    return result;
}

std::string FindValue(const KeyValueMap& map, const auto& keys) {
    for (const auto& key : keys) {
        auto it = map.find(std::string{key});
        if (it != map.end()) {
            return it->second;
        }
    }
    return {};
}

bool IsOneOf(std::string_view key, const auto& keys) {
    return std::find(std::begin(keys), std::end(keys), key) != std::end(keys);
}

void AppendWithValue(std::string& output, const DsnPart& part, std::string_view value) {
    output.append(part.raw.substr(0, part.value_begin));
    output.append(value);
    output.append(part.raw.substr(part.value_end));
}

}  // namespace

DsnOptions ParseDsn(const Dsn& dsn) {
    auto map = ParseDsnToMap(dsn.GetUnderlying());

    DsnOptions opts;
    opts.driver = FindValue(map, kDriverKeys);
    opts.server = FindValue(map, kServerKeys);
    opts.port = FindValue(map, kPortKeys);
    opts.database = FindValue(map, kDatabaseKeys);
    opts.uid = FindValue(map, kUserKeys);

    return opts;
}

std::string GetHostPort(const Dsn& dsn) {
    auto opts = ParseDsn(dsn);
    if (opts.server.empty()) {
        return {};
    }
    if (opts.port.empty()) {
        return opts.server;
    }
    return opts.server + ":" + opts.port;
}

std::string DsnCutPassword(const Dsn& dsn) {
    std::string result;
    result.reserve(dsn.GetUnderlying().size());
    for (const auto& part : ParseDsnParts(dsn.GetUnderlying())) {
        if (!part.valid || !IsOneOf(part.key, kPasswordKeys)) {
            result.append(part.raw);
        }
    }
    return result;
}

std::string DsnMaskPassword(const Dsn& dsn) {
    std::string result;
    result.reserve(dsn.GetUnderlying().size());
    for (const auto& part : ParseDsnParts(dsn.GetUnderlying())) {
        if (part.valid && IsOneOf(part.key, kPasswordKeys)) {
            AppendWithValue(result, part, "***");
        } else {
            result.append(part.raw);
        }
    }
    return result;
}

bool IsIpAddress(std::string_view host) {
    if (host.empty()) {
        return false;
    }

    // IPv4 check: all digits and dots
    bool could_be_ipv4 = true;
    int dot_count = 0;
    for (char c : host) {
        if (c == '.') {
            ++dot_count;
        } else if (!std::isdigit(static_cast<unsigned char>(c))) {
            could_be_ipv4 = false;
            break;
        }
    }
    if (could_be_ipv4 && dot_count == 3) {
        return true;
    }

    // IPv6 check: contains colons
    if (host.find(':') != std::string_view::npos) {
        return true;
    }

    return false;
}

Dsn ResolveDsnHost(const Dsn& dsn, clients::dns::Resolver& resolver, engine::Deadline deadline) {
    const auto opts = ParseDsn(dsn);
    const auto& server = opts.server;

    if (server.empty() || IsIpAddress(server)) {
        return dsn;
    }

    auto addrs = resolver.Resolve(server, deadline);
    if (addrs.empty()) {
        throw ConnectionError("Failed to resolve hostname: " + server);
    }

    auto resolved_ip = addrs.front().PrimaryAddressString();
    LOG_DEBUG() << "Resolved ODBC host " << server << " to " << resolved_ip;

    return detail::ReplaceDsnHost(dsn, resolved_ip);
}

namespace detail {

Dsn ReplaceDsnHost(const Dsn& dsn, std::string_view resolved_host) {
    const auto parts = ParseDsnParts(dsn.GetUnderlying());
    std::optional<std::size_t> target;
    for (const auto& key : kServerKeys) {
        for (std::size_t index = 0; index < parts.size(); ++index) {
            if (parts[index].valid && parts[index].key == key && !parts[index].value.empty()) {
                target = index;
            }
        }
        if (target) {
            break;
        }
    }
    if (!target) {
        return dsn;
    }

    std::string result;
    result.reserve(dsn.GetUnderlying().size() + resolved_host.size());
    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index == *target) {
            AppendWithValue(result, parts[index], resolved_host);
        } else {
            result.append(parts[index].raw);
        }
    }
    return Dsn{std::move(result)};
}

}  // namespace detail

}  // namespace storages::odbc

USERVER_NAMESPACE_END
