#include <userver/storages/scylla/component.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/storages/scylla/exception.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/storages/scylla/session_config.hpp>
#include <userver/utils/algo.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <storages/scylla/scylla_secdist.hpp>

#include <userver/storages/secdist/component.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/scylla/component.yaml.hpp"
#endif

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

Scylla::Scylla(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context)
{
    auto db_alias = config["dbalias"].As<std::string>("");

    std::string hosts;

    storages::secdist::Secdist* secdist{};
    if (!db_alias.empty()) {
        dbalias_ = db_alias;
        secdist = &context.FindComponent<Secdist>().GetStorage();
        hosts = storages::scylla::secdist::GetSecdistHosts(*secdist, dbalias_);
    } else {
        hosts = config["dbconnection"].As<std::string>();
    }

    if (hosts.empty()) {
        throw storages::scylla::InvalidConfigException(
            utils::StrCat(config.Name(), ": either 'dbalias' or 'dbconnection' must be set in static config")
        );
    }

    auto* dns_resolver = clients::dns::GetResolverPtr(config, context);

    const auto dynamic_config = context.FindComponent<DynamicConfig>().GetSource();
    const auto session_config = ParseSessionConfig(config);

    session_ = std::make_shared<
        storages::scylla::Session>(config.Name(), hosts, session_config, dynamic_config, dns_resolver);

    if (!dbalias_.empty()) {
        secdist_subscriber_ = secdist->UpdateAndListen(this, dbalias_, &Scylla::OnSecdistUpdate);
    }

    auto section_name = config.Name();
    if (boost::algorithm::starts_with(section_name, kStandardScyllaPrefix) &&
        section_name.size() != kStandardScyllaPrefix.size())
    {
        section_name = section_name.substr(kStandardScyllaPrefix.size());
    }

    utils::statistics::RegisterWriterScope(
        context,
        "scylla",
        [this](utils::statistics::Writer& writer) {
            UASSERT(session_);
            DumpMetric(writer, *session_);
        },
        {{"scylla_database", std::move(section_name)}}
    );
}

storages::scylla::SessionPtr Scylla::GetSession() const { return session_; }

yaml_config::Schema Scylla::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/scylla/component.yaml");
}

void Scylla::OnSecdistUpdate(const storages::secdist::SecdistConfig& config) {
    auto hosts = storages::scylla::secdist::GetSecdistHosts(config, dbalias_);
    session_->SetContactPoints(hosts);
}

Scylla::~Scylla() { secdist_subscriber_.Unsubscribe(); }
}  // namespace components

USERVER_NAMESPACE_END
