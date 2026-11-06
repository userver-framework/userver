#pragma once

/// @file userver/components/statistics_storage.hpp
/// @brief @copybrief components::StatisticsStorage

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component that keeps a @ref utils::statistics::Storage storage for metrics.
///
/// Returned references to @ref utils::statistics::Storage live for a lifetime
/// of the component and are safe for concurrent use.
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet core/src/components/common_component_list_test.cpp  Sample statistics storage component config
class StatisticsStorage final : public RawComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of components::StatisticsStorage component
    static constexpr std::string_view kName = "statistics-storage";

    StatisticsStorage(const ComponentConfig& config, const ComponentContext& context);

    ~StatisticsStorage() override;

    void OnAllComponentsLoaded() override;

    utils::statistics::Storage& GetStorage() { return storage_; }

    const utils::statistics::Storage& GetStorage() const { return storage_; }

    utils::statistics::MetricsStoragePtr GetMetricsStorage() { return metrics_storage_; }

    utils::statistics::MetricsStorage& GetMetricsStorageRef() {
        UASSERT(metrics_storage_ != nullptr);
        return *metrics_storage_;
    }

    static yaml_config::Schema GetStaticConfigSchema();

private:
    utils::statistics::Storage storage_;
    utils::statistics::MetricsStoragePtr metrics_storage_;
};

template <>
inline constexpr bool kHasValidate<StatisticsStorage> = true;

template <>
inline constexpr auto kConfigFileMode<StatisticsStorage> = ConfigFileMode::kNotRequired;

}  // namespace components

namespace utils::statistics {

/// @brief Registers a writer in the default @ref components::StatisticsStorage.
/// Call this helper in the component constructor. Registration is deferred until the complete component, including
/// derived classes, has been constructed. The callback is unregistered before the component destructor starts.
/// The callback may run concurrently with other code and must be thread-safe.
///
/// @see utils::statistics::Storage::RegisterWriter
void RegisterWriterScope(
    const components::ComponentContext&,
    std::string common_prefix,
    WriterFunc func,
    std::vector<Label> add_labels = {}
);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
