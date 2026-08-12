#include "dsn.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

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

KeyValueMap ParseDsnToMap(std::string_view dsn) {
    KeyValueMap result;

    std::string current_key;
    std::string current_value;
    bool in_braces = false;
    bool reading_value = false;

    for (char c : dsn) {
        if (c == '{' && !in_braces && reading_value) {
            in_braces = true;
            continue;
        }

        if (c == '}' && in_braces) {
            in_braces = false;
            continue;
        }

        if (c == '=' && !in_braces && !reading_value) {
            reading_value = true;
            continue;
        }

        if (c == ';' && !in_braces) {
            if (!current_key.empty()) {
                result[utils::text::ToUpper(current_key)] = current_value;
            }
            current_key.clear();
            current_value.clear();
            reading_value = false;
            continue;
        }

        if (reading_value) {
            current_value += c;
        } else {
            current_key += c;
        }
    }

    if (!current_key.empty()) {
        result[utils::text::ToUpper(current_key)] = current_value;
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

std::string RebuildDsn(const KeyValueMap& map) {
    std::string result;
    result.reserve(128);
    for (const auto& [key, value] : map) {
        if (!result.empty()) {
            result += ';';
        }

        const bool needs_braces =
            (value.find(';') != std::string::npos || value.find('{') != std::string::npos ||
             value.find('}') != std::string::npos);

        result += key;
        result += '=';
        if (needs_braces) {
            result += '{';
        }
        result += value;
        if (needs_braces) {
            result += '}';
        }
    }

    return result;
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
    auto map = ParseDsnToMap(dsn.GetUnderlying());

    for (const auto& key : kPasswordKeys) {
        map.erase(std::string{key});
    }

    return RebuildDsn(map);
}

std::string DsnMaskPassword(const Dsn& dsn) {
    auto map = ParseDsnToMap(dsn.GetUnderlying());

    for (const auto& key : kPasswordKeys) {
        auto it = map.find(std::string{key});
        if (it != map.end()) {
            it->second = "***";
        }
    }

    return RebuildDsn(map);
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
    auto map = ParseDsnToMap(dsn.GetUnderlying());

    std::string server;
    std::string server_key;
    for (const auto& key : kServerKeys) {
        auto it = map.find(std::string{key});
        if (it != map.end() && !it->second.empty()) {
            server = it->second;
            server_key = std::string{key};
            break;
        }
    }

    if (server.empty() || IsIpAddress(server)) {
        return dsn;
    }

    auto addrs = resolver.Resolve(server, deadline);
    if (addrs.empty()) {
        throw ConnectionError("Failed to resolve hostname: " + server);
    }

    auto resolved_ip = addrs.front().PrimaryAddressString();
    LOG_DEBUG() << "Resolved ODBC host " << server << " to " << resolved_ip;

    map[server_key] = resolved_ip;

    return Dsn{RebuildDsn(map)};
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
