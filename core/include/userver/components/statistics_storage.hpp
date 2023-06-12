#pragma once

/// @file userver/components/statistics_storage.hpp
/// @brief @copybrief components::StatisticsStorage

#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that keeps a utils::statistics::Storage storage for
/// metrics.
///
/// Returned references to utils::statistics::Storage live for a lifetime
/// of the component and are safe for concurrent use.
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample statistics storage component config

// clang-format on
class StatisticsStorage final : public impl::ComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::StatisticsStorage component
  static constexpr std::string_view kName = "statistics-storage";

  StatisticsStorage(const ComponentConfig& config,
                    const ComponentContext& context);

  ~StatisticsStorage() override;

  void OnAllComponentsLoaded() override;

  utils::statistics::Storage& GetStorage() { return storage_; }

  const utils::statistics::Storage& GetStorage() const { return storage_; }

  utils::statistics::MetricsStoragePtr GetMetricsStorage() {
    return metrics_storage_;
  }

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  utils::statistics::Storage storage_;
  utils::statistics::MetricsStoragePtr metrics_storage_;
  std::vector<utils::statistics::Entry> metrics_storage_registration_;
};

template <>
inline constexpr bool kHasValidate<StatisticsStorage> = true;

template <>
inline constexpr auto kConfigFileMode<StatisticsStorage> =
    ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
