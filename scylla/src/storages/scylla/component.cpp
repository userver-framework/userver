#include <iostream>

#include <userver/storages/scylla/component.hpp>

#include <userver/components/component.hpp>
#include <userver/storages/scylla/exception.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/scylla/component.yaml.hpp"  // Y_IGNORE
#endif

#include "userver/storages/secdist/component.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/storages/scylla/session_config.hpp>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/statistics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {
const std::string kStandardScyllaPrefix = "scylla-";

storages::scylla::SessionConfig ParseSessionConfig(const ComponentConfig& config) {
    auto session_config = config.As<storages::scylla::SessionConfig>();

    session_config.Validate(config.Name());

    return session_config;
}
}  // namespace

Scylla::Scylla(const ComponentConfig& config, const ComponentContext& context) : ComponentBase(config, context) {
    auto db_alias = config["dbalias"].As<std::string>("");

    std::string hosts;

    storages::secdist::Secdist* secdist{};
    if (!db_alias.empty()) {
        dbalias_ = db_alias;
        secdist = &context.FindComponent<Secdist>().GetStorage();

        hosts = "???";
    } else {
        // TODO: rename to hosts
        hosts = config["dbconnection"].As<std::string>();
    }

    if (hosts.empty()) {
        throw storages::scylla::InvalidConfigException(
            config.Name() + ": either 'dbalias' or 'dbconnection' must be set in static config");
    }

    auto* dns_resolver = clients::dns::GetResolverPtr(config, context);

    const auto dynamic_config = context.FindComponent<DynamicConfig>().GetSource();
    const auto session_config = ParseSessionConfig(config);

    session_ = std::make_shared<
        storages::scylla::Session>(config.Name(), hosts, session_config, dynamic_config, dns_resolver);

    const auto& statistics_storage = context.FindComponent<components::StatisticsStorage>();

    auto component_name = config.Name();

    const bool has_name_after_prefix = component_name.size() > kStandardScyllaPrefix.size();
    const bool has_scylla_prefix = boost::algorithm::starts_with(component_name, kStandardScyllaPrefix);

    if (has_scylla_prefix && has_name_after_prefix) {
        component_name = component_name.substr(kStandardScyllaPrefix.size());
    }
};

storages::scylla::SessionPtr Scylla::GetSession() const { return session_; }

yaml_config::Schema Scylla::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/scylla/component.yaml");
}

Scylla::~Scylla() = default;
}  // namespace components

USERVER_NAMESPACE_END