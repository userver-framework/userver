#pragma once

/// @file userver/utils/statistics/metrics_storage.hpp
/// @brief @copybrief utils::statistics::MetricsStorage

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/metric_tag.hpp>
#include <userver/utils/statistics/metric_tag_impl.hpp>

// TODO(USERVER-1855) remove extra includes
#include <cstddef>
#include <typeinfo>
#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Storage of metrics registered with MetricTag<Metric>
/// @note The class is thread-safe. See also the note about thread-safety
/// on MetricTag<Metric>.
class MetricsStorage final {
public:
    /// @brief Creates `MetricStorage` that considers all `MetricTag<Metric>` instances
    MetricsStorage();

    /// @brief Creates `MetricStorage` that considers only `MetricTag<Metric>` instances whose paths are listed in
    /// `allowed_metric_paths`. If `allowed_metric_paths` equals `std::nullopt`, then all `MetricTag<Metric>` instances
    /// considered
    /// @param allowed_metric_paths paths of `MetricTag<Metric>` instances that should be considered or
    /// `std::nullopt` if all `MetricTag<Metric>` instances should be considered
    explicit MetricsStorage(const std::optional<std::vector<std::string>>& allowed_metric_paths);

    MetricsStorage(const MetricsStorage&) = delete;
    MetricsStorage(MetricsStorage&& other) noexcept;
    MetricsStorage& operator=(const MetricsStorage&) = delete;
    MetricsStorage& operator=(MetricsStorage&& other) noexcept;
    ~MetricsStorage();

    /// Get metric data by type
    template <typename Metric>
    Metric& GetMetric(const MetricTag<Metric>& tag) {
        return dynamic_cast<impl::MetricWrapper<Metric>&>(GetMetricWrapper(tag.key_)).Get();
    }

    /// @brief Dumps all the metrics of @a storage through @a writer, each under its `MetricTag` path.
    ///
    /// Makes `MetricsStorage` satisfy @ref utils::statistics::HasWriterSupport, so it can be passed directly to
    /// @ref utils::statistics::Snapshot.
    ///
    /// @note A `MetricsStorage` instantiates every `MetricTag<Metric>` registered in the binary, not only the ones
    /// the test cares about. When building a @ref utils::statistics::Snapshot, always pass
    /// @ref utils::statistics::SnapshotBuildOptions::require_prefix to filter out unrelated metrics; otherwise the
    /// snapshot collects all `MetricTag` instances of the binary, which slows the test down and makes it depend on
    /// unrelated registrations:
    /// @snippet core/src/utils/statistics/metrics_storage_test.cpp  metrics storage Snapshot require_prefix
    friend void DumpMetric(Writer& writer, const MetricsStorage& storage);

    void ResetMetrics();

private:
    impl::MetricWrapperBase& GetMetricWrapper(const impl::MetricKey& key);

    struct Impl;
    utils::FastPimpl<Impl, 64, alignof(void*)> impl_;
};

using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

/// @brief Creates a @ref MetricsStorage that considers only `MetricTag<Metric>` instances whose paths are listed in
/// @a allowed_metric_paths (or all of them, if it is `std::nullopt`), and registers it in @a storage using the scoped
/// @ref utils::statistics::Storage::RegisterWriter overload, with an empty prefix.
///
/// The returned pointer is an aliasing `shared_ptr`: it keeps alive an internal holder that unregisters the writer
/// when the last pointer is destroyed. The @a storage must outlive the returned pointer (and all its copies).
MetricsStoragePtr MakeBoundMetricsStorage(
    Storage& storage,
    const std::optional<std::vector<std::string>>& allowed_metric_paths
);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
