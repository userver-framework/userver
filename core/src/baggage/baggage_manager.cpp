#include <userver/baggage/baggage_manager.hpp>
#include <userver/baggage/baggage_settings.hpp>

#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace baggage {

namespace {

std::unordered_set<std::string> ChooseCurrentAllowedKeys(
    const Baggage* current_baggage,
    const dynamic_config::Source& config_source) {
  if (current_baggage != nullptr) {
    return current_baggage->GetAllowedKeys();
  }
  const auto snapshot = config_source.GetSnapshot();
  const auto& baggage_settings = snapshot[kBaggageSettings];
  return baggage_settings.allowed_keys;
}

}  // namespace

BaggageManagerComponent::BaggageManagerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      baggage_manager_(
          context.FindComponent<components::DynamicConfig>().GetSource()) {}

BaggageManager& BaggageManagerComponent::GetManager() {
  return baggage_manager_;
}

yaml_config::Schema BaggageManagerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component for interaction with Baggage header.
additionalProperties: false
properties: {}
)");
}

BaggageManager::BaggageManager(const dynamic_config::Source& config_source)
    : config_source_(config_source) {}

/// @brief Returns if baggage is enabled
bool BaggageManager::IsEnabled() const {
  return config_source_.GetCopy(kBaggageEnabled);
}

void BaggageManager::AddEntry(std::string key, std::string value,
                              BaggageProperties properties) const {
  if (!IsEnabled()) {
    return;
  }
  const auto* current_baggage = TryGetBaggage();

  auto baggage = current_baggage
                     ? std::move(*current_baggage)
                     : Baggage("", ChooseCurrentAllowedKeys(current_baggage,
                                                            config_source_));

  baggage.AddEntry(std::move(key), std::move(value), std::move(properties));
  kInheritedBaggage.Set(std::move(baggage));
}

const Baggage* BaggageManager::TryGetBaggage() {
  return kInheritedBaggage.GetOptional();
}

void BaggageManager::SetBaggage(std::string header) const {
  if (!IsEnabled()) {
    return;
  }
  const auto* current_baggage = TryGetBaggage();
  auto current_allowed_keys =
      ChooseCurrentAllowedKeys(current_baggage, config_source_);

  auto baggage =
      TryMakeBaggage(std::move(header), std::move(current_allowed_keys));
  if (baggage) {
    kInheritedBaggage.Set(std::move(*baggage));
  }
}

void BaggageManager::ResetBaggage() { kInheritedBaggage.Erase(); }

}  // namespace baggage

USERVER_NAMESPACE_END
