#pragma once

/// @file userver/components/statistics_storage.hpp
/// @brief @copybrief components::StatisticsStorage

#include "loggable_component_base.hpp"

#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/yaml_config/schema.hpp>

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
class StatisticsStorage final : public LoggableComponentBase {
 public:
  static constexpr auto kName = "statistics-storage";

  StatisticsStorage(const ComponentConfig& config,
                    const components::ComponentContext&);

  ~StatisticsStorage() override;

  void OnAllComponentsLoaded() override;

  utils::statistics::Storage& GetStorage() { return storage_; }

  const utils::statistics::Storage& GetStorage() const { return storage_; }

  utils::statistics::MetricsStoragePtr GetMetricsStorage() {
    return metrics_storage_;
  }

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  formats::json::ValueBuilder ExtendStatistics(
      const utils::statistics::StatisticsRequest&);

  utils::statistics::Storage storage_;
  utils::statistics::MetricsStoragePtr metrics_storage_;
  utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<StatisticsStorage> = true;

}  // namespace components

USERVER_NAMESPACE_END
