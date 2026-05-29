#include "scylla_secdist.hpp"

#include <functional>
#include <unordered_map>

#include <userver/formats/json/value.hpp>
#include <userver/storages/scylla/exception.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include <userver/utils/str_icase.hpp>

#include <boost/range/adaptor/map.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::secdist {
namespace {

std::optional<SslSecrets> ParseSslSecrets(const formats::json::Value& ssl_val) {
    if (ssl_val.IsMissing()) {
        return std::nullopt;
    }

    SslSecrets result;
    if (ssl_val.HasMember("trusted_certs")) {
        for (const auto& cert : ssl_val["trusted_certs"]) {
            result.trusted_certs.push_back(cert.As<std::string>());
        }
    }
    if (ssl_val.HasMember("client_cert")) {
        result.client_cert = ssl_val["client_cert"].As<std::string>();
    }
    if (ssl_val.HasMember("client_key")) {
        result.client_key = ssl_val["client_key"].As<std::string>();
    }
    result.client_key_password = ssl_val["client_key_password"].As<std::string>("");
    return result;
}

struct ScyllaDbSettings {
    std::string hosts;
    std::optional<SslSecrets> ssl;
};

class ScyllaSettings {
public:
    explicit ScyllaSettings(const formats::json::Value& doc);

    const ScyllaDbSettings& Get(std::string_view dbalias) const;

private:
    std::unordered_map<std::string, ScyllaDbSettings, utils::StrCaseHash, utils::StrIcaseEqual> settings_;
};

ScyllaSettings::ScyllaSettings(const formats::json::Value& doc) {
    const formats::json::Value& scylla_settings = doc["scylla_settings"];
    if (scylla_settings.IsMissing()) {
        return;
    }

    storages::secdist::CheckIsObject(scylla_settings, "scylla_settings");

    for (auto it = scylla_settings.begin(); it != scylla_settings.end(); ++it) {
        const std::string& dbalias = it.GetName();
        const formats::json::Value& dbsettings = *it;
        storages::secdist::CheckIsObject(dbsettings, "dbsettings");

        ScyllaDbSettings entry;
        entry.hosts = storages::secdist::GetString(dbsettings, "hosts");
        entry.ssl = ParseSslSecrets(dbsettings["ssl"]);
        settings_[dbalias] = std::move(entry);
    }
}

const ScyllaDbSettings& ScyllaSettings::Get(std::string_view dbalias) const {
    auto it = settings_.find(dbalias);

    if (it == settings_.end()) {
        throw storages::scylla::InvalidConfigException(fmt::format(
            "dbalias {} not found in secdist config. Available aliases: [{}]",
            dbalias,
            fmt::join(settings_ | boost::adaptors::map_keys, ", ")
        ));
    }

    return it->second;
}

}  // namespace

std::string GetSecdistHosts(const storages::secdist::Secdist& secdist, std::string_view dbalias) {
    auto snapshot = secdist.GetSnapshot();
    return GetSecdistHosts(*snapshot, dbalias);
}

std::string GetSecdistHosts(const storages::secdist::SecdistConfig& secdist, std::string_view dbalias) {
    try {
        return secdist.Get<storages::scylla::secdist::ScyllaSettings>().Get(dbalias).hosts;
    } catch (const storages::secdist::SecdistError& ex) {
        throw storages::scylla::InvalidConfigException("Failed to load scylla config for dbalias "
        ) << dbalias
          << ": " << ex.what();
    }
}

std::optional<SslSecrets> GetSecdistSsl(const storages::secdist::Secdist& secdist, std::string_view dbalias) {
    auto snapshot = secdist.GetSnapshot();
    return GetSecdistSsl(*snapshot, dbalias);
}

std::optional<SslSecrets> GetSecdistSsl(const storages::secdist::SecdistConfig& secdist, std::string_view dbalias) {
    try {
        return secdist.Get<storages::scylla::secdist::ScyllaSettings>().Get(dbalias).ssl;
    } catch (const storages::secdist::SecdistError& ex) {
        throw storages::scylla::InvalidConfigException("Failed to load scylla SSL config for dbalias "
        ) << dbalias
          << ": " << ex.what();
    }
}

}  // namespace storages::scylla::secdist

USERVER_NAMESPACE_END
