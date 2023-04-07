#pragma once

#include <userver/baggage/baggage.hpp>
#include <userver/baggage/baggage_settings.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace baggage {

class BaggageManager final {
 public:
  explicit BaggageManager(const BaggageSettings& settings);

  /// @brief Add entry to baggage header.
  /// @throws BaggageException If key, value or properties
  /// don't match with requirements or if allowed_keys
  /// don't contain selected key
  void AddEntry(std::string key, std::string value,
                BaggageProperties properties) const;

  /// @brief Get const reference to baggage value from task inherited variable
  static const Baggage& GetBaggage();

  /// @brief Set new baggage value to task inherited variable
  void SetBaggage(std::string header) const;

  /// @brief Clear header's data.
  void ResetBaggage() const;

 private:
  BaggageSettings settings_;
};

/// @brief manager for relationship with header baggage.
class BaggageManagerComponent final : public components::LoggableComponentBase {
 public:
  static constexpr const char* kName = "baggage-manager";

  BaggageManagerComponent(const components::ComponentConfig& config,
                          const components::ComponentContext& context);

  BaggageManager GetManager();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  dynamic_config::Source config_source_;
};

}  // namespace baggage

USERVER_NAMESPACE_END
