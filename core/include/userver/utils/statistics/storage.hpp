#pragma once

/// @file userver/utils/statistics/storage.hpp
/// @brief @copybrief utils::statistics::Storage

#include <atomic>
#include <functional>
#include <list>
#include <string>
#include <vector>

#include <userver/engine/shared_mutex.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/resource_scopes_fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

using WriterFunc = std::function<void(Writer&)>;

namespace impl {

struct MetricsSource final {
    std::string prefix_path;
    WriterFunc writer;
    std::vector<Label> writer_labels;
};

using StorageData = std::list<MetricsSource>;
using StorageIterator = StorageData::iterator;

inline constexpr bool kCheckSubscriptionUB = utils::impl::kEnableAssert;

}  // namespace impl

/// @ingroup userver_clients
///
/// Storage of metrics, usually retrieved from components::StatisticsStorage.
///
/// See @ref utils::statistics::Writer for an information on how to write metrics.
///
/// For introduction to metrics see @ref scripts/docs/en/userver/metrics.md
class Storage final {
public:
    Storage();

    Storage(const Storage&) = delete;

    /// @cond
    /// Must be called from StatisticsStorage only. Don't call it from user components.
    void StopRegisteringExtenders();
    /// @endcond

    /// @brief Add a writer function @b func. Note that `func` is called concurrently with
    /// other code, so it should be thread-safe.
    ///
    /// Registration runs after construction via @ref ResourceScopeStorage::AfterConstruction.
    /// Unregister runs in @ref ResourceScopeStorage::BeforeDestruction.
    /// The storage must outlive its registrations. If other scopes initialize or destroy data accessed by `func`
    /// (generally discouraged), register the statistics scope after them so the data is initialized before the callback
    /// is registered and destroyed after it is unregistered.
    ///
    /// In a component constructor prefer @ref RegisterWriterScope with @ref components::ComponentContext.
    ///
    /// @param scopes storage that owns the writer lifetime.
    /// See @ref resource_scopes_parameter "where to obtain scopes".
    /// @param common_prefix prefix for the metric, for example "my.metric_name"
    /// @param func function that writes metrics to @ref utils::statistics::Writer
    /// @param add_labels common labels for the metric, for example {"database", "dbname"}
    void RegisterWriter(
        ResourceScopeStorage& scopes,
        std::string common_prefix,
        WriterFunc func,
        std::vector<Label> add_labels = {}
    );

private:
    friend void DumpMetric(Writer& writer, const Storage& storage);

    impl::StorageIterator DoRegisterWriter(impl::MetricsSource&& source);
    void DoUnregisterWriter(impl::StorageIterator iterator) noexcept;

    std::atomic<bool> may_register_extenders_;
    impl::StorageData metrics_sources_;
    mutable engine::SharedMutex mutex_;
};

/// @brief Dumps metrics registered in @a storage.
void DumpMetric(Writer& writer, const Storage& storage);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
