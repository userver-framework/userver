#pragma once

/// @file userver/alert/component.hpp
/// @brief @copybrief alert::Component

#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>

#include <userver/alerts/storage.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Alerts management
namespace alerts {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that keeps an alert::Storage storage for
/// fired alerts.
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample statistics storage component config

// clang-format on
class StorageComponent final : public components::impl::ComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of alert::StorageComponent component
  static constexpr std::string_view kName = "alerts-storage";

  StorageComponent(const components::ComponentConfig& config,
                   const components::ComponentContext& context);

  Storage& GetStorage() const;

 private:
  mutable Storage storage_;
};

}  // namespace alerts

template <>
inline constexpr bool components::kHasValidate<alerts::StorageComponent> = true;

template <>
inline constexpr auto components::kConfigFileMode<alerts::StorageComponent> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
