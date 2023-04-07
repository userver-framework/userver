#include <userver/baggage/baggage_manager.hpp>
#include <userver/baggage/baggage_settings.hpp>

#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace baggage {

BaggageManagerComponent::BaggageManagerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      config_source_(
          context.FindComponent<components::DynamicConfig>().GetSource()) {}

BaggageManager BaggageManagerComponent::GetManager() {
  auto baggage_settings = config_source_.GetCopy(kBaggageSettings);
  return BaggageManager(baggage_settings);
}

yaml_config::Schema BaggageManagerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component for interaction with Baggage header.
additionalProperties: false
properties: {}
)");
}

BaggageManager::BaggageManager(const BaggageSettings& settings)
    : settings_(settings) {}

void BaggageManager::AddEntry(std::string key, std::string value,
                              BaggageProperties properties) const {
  const auto* baggage_ptr = kInheritedBaggage.GetOptional();
  if (baggage_ptr == nullptr) {
    throw BaggageException("Baggage inherited variable doesn't exist");
  }

  auto baggage_copy =
      TryMakeBaggage(baggage_ptr->ToString(), settings_.allowed_keys);
  if (!baggage_copy) {
    throw BaggageException("Exception while copying baggage");
  }

  baggage_copy->AddEntry(std::move(key), std::move(value),
                         std::move(properties));
  kInheritedBaggage.Set(std::move(*baggage_copy));
}

const Baggage& BaggageManager::GetBaggage() {
  const auto* baggage_ptr = kInheritedBaggage.GetOptional();
  if (baggage_ptr == nullptr) {
    throw BaggageException("Baggage inherited variable doesn't exist");
  }
  return *baggage_ptr;
}

void BaggageManager::SetBaggage(std::string header) const {
  auto baggage = TryMakeBaggage(std::move(header), settings_.allowed_keys);
  if (baggage) {
    kInheritedBaggage.Set(std::move(*baggage));
  }
}

void BaggageManager::ResetBaggage() const {
  auto baggage = TryMakeBaggage("", settings_.allowed_keys);
  kInheritedBaggage.Set(std::move(*baggage));
}

}  // namespace baggage

USERVER_NAMESPACE_END
