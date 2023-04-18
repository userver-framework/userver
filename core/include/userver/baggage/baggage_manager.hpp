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
  explicit BaggageManager(const dynamic_config::Source& config_source);

  /// @brief Returns if baggage is enabled
  bool IsEnabled() const;

  /// @brief Add entry to baggage header.
  /// @throws BaggageException If key, value or properties
  /// don't match with requirements or if allowed_keys
  /// don't contain selected key
  void AddEntry(std::string key, std::string value,
                BaggageProperties properties) const;

  /// @brief Get const pointer to baggage value from task inherited variable
  static const Baggage* TryGetBaggage();

  /// @brief Set new baggage value to task inherited variable
  void SetBaggage(std::string header) const;

  /// @brief Delete header from task inherited variable
  static void ResetBaggage();

 private:
  dynamic_config::Source config_source_;
};

/// @brief manager for relationship with header baggage.
class BaggageManagerComponent final : public components::LoggableComponentBase {
 public:
  static constexpr const char* kName = "baggage-manager";

  BaggageManagerComponent(const components::ComponentConfig& config,
                          const components::ComponentContext& context);

  BaggageManager& GetManager();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  BaggageManager baggage_manager_;
};

}  // namespace baggage

USERVER_NAMESPACE_END
