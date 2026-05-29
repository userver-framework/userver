#include "odbc_secdist.hpp"

#include <ranges>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::secdist {

OdbcSettings::OdbcSettings(const formats::json::Value& doc) {
    const formats::json::Value& odbc_settings = doc["odbc_settings"];
    if (odbc_settings.IsMissing()) {
        LOG_WARNING() << "'odbc_settings' secdist section is empty";
        return;
    }

    storages::secdist::CheckIsObject(odbc_settings, "odbc_settings");

    const formats::json::Value& databases = odbc_settings["databases"];
    if (databases.IsMissing()) {
        LOG_WARNING() << "'odbc_settings.databases' secdist section is empty";
        return;
    }

    storages::secdist::CheckIsObject(databases, "databases");

    for (const auto& [dbalias, db_config] : formats::common::Items(databases)) {
        storages::secdist::CheckIsObject(db_config, dbalias);

        std::vector<OdbcConnectionInfo> connections;

        // Support both single DSN and array of DSNs
        if (db_config.HasMember("dsn")) {
            // Single DSN format
            OdbcConnectionInfo info;
            info.dsn = db_config["dsn"].As<std::string>();
            connections.push_back(std::move(info));
        } else if (db_config.HasMember("hosts")) {
            // Array of hosts format (like MySQL)
            const auto& hosts = db_config["hosts"];
            storages::secdist::CheckIsArray(hosts, dbalias + ".hosts");

            for (auto host_it = hosts.begin(); host_it != hosts.end(); ++host_it) {
                if (host_it->IsString()) {
                    // Simple DSN string
                    OdbcConnectionInfo info;
                    info.dsn = host_it->As<std::string>();
                    connections.push_back(std::move(info));
                } else if (host_it->IsObject()) {
                    // Object with dsn field
                    OdbcConnectionInfo info;
                    info.dsn = (*host_it)["dsn"].As<std::string>();
                    connections.push_back(std::move(info));
                } else {
                    storages::secdist::ThrowInvalidSecdistType(*host_it, "a string or object");
                }
            }
        } else {
            throw storages::secdist::SecdistError(
                fmt::format("Database '{}' must have either 'dsn' or 'hosts' field", dbalias)
            );
        }

        if (connections.empty()) {
            throw storages::secdist::SecdistError(fmt::format("Database '{}' has no connection info", dbalias));
        }

        databases_[dbalias] = std::move(connections);
    }
}

std::vector<OdbcConnectionInfo> OdbcSettings::GetConnectionInfos(const std::string& dbalias) const {
    auto it = databases_.find(dbalias);
    if (it == databases_.end()) {
        throw storages::secdist::SecdistError(fmt::format(
            "dbalias '{}' not found in secdist config. Available aliases: [{}]",
            dbalias,
            fmt::join(databases_ | std::views::keys, ", ")
        ));
    }
    return it->second;
}

}  // namespace storages::odbc::secdist

USERVER_NAMESPACE_END
