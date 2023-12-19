#include <userver/storages/secdist/component.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/provider.hpp>
#include <userver/utils/string_to_duration.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

storages::secdist::SecdistConfig::Settings ParseSettings(
    const ComponentConfig& config, const ComponentContext& context) {
  storages::secdist::SecdistConfig::Settings settings;

  const auto provider_name =
      config["provider"].As<std::string>("default-secdist-provider");
  settings.provider =
      &context.FindComponent<storages::secdist::SecdistProvider>(provider_name);
  settings.update_period =
      utils::StringToDuration(config["update-period"].As<std::string>("0s"));
  return settings;
}

}  // namespace

Secdist::Secdist(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      secdist_(ParseSettings(config, context)) {}

const storages::secdist::SecdistConfig& Secdist::Get() const {
  return secdist_.Get();
}

rcu::ReadablePtr<storages::secdist::SecdistConfig> Secdist::GetSnapshot()
    const {
  return secdist_.GetSnapshot();
}

storages::secdist::Secdist& Secdist::GetStorage() { return secdist_; }

yaml_config::Schema Secdist::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component that stores security related data (keys, passwords, ...).
additionalProperties: false
properties:
    update-period:
        type: string
        description: period between data updates in utils::StringToDuration() suitable format ('0s' for no updates)
        defaultDescription: 0s
    provider:
        type: string
        description: optional secdist provider component name
        defaultDescription: default-secdist-provider
)");
}

}  // namespace components

USERVER_NAMESPACE_END
