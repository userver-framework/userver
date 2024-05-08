#include <userver/storages/secdist/component_base.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/default_provider.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/utils/string_to_duration.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

SecdistComponentBase::SecdistComponentBase(
    const ComponentConfig& config, const ComponentContext& context,
    storages::secdist::SecdistConfig::Settings&& settings)
    : LoggableComponentBase(config, context), secdist_(std::move(settings)) {}

const storages::secdist::SecdistConfig& SecdistComponentBase::Get() const {
  return secdist_.Get();
}

rcu::ReadablePtr<storages::secdist::SecdistConfig>
SecdistComponentBase::GetSnapshot() const {
  return secdist_.GetSnapshot();
}

storages::secdist::Secdist& SecdistComponentBase::GetStorage() {
  return secdist_;
}

yaml_config::Schema SecdistComponentBase::GetStaticConfigSchema() {
  auto schema = LoggableComponentBase::GetStaticConfigSchema();
  schema.UpdateDescription(
      "Base class for user defined secdists and DefaultSecdist");
  return schema;
}

}  // namespace components

USERVER_NAMESPACE_END
