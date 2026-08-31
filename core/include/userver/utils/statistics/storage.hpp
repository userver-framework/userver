#pragma once

/// @file userver/utils/statistics/storage.hpp
/// @brief @copybrief utils::statistics::Storage

#include <atomic>
#include <functional>
#include <list>
#include <string>
#include <vector>

#include <userver/engine/shared_mutex.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/resource_scopes_fwd.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Used in legacy statistics extenders
struct StatisticsRequest final {};

using ExtenderFunc = std::function<formats::json::ValueBuilder(const StatisticsRequest&)>;

using WriterFunc = std::function<void(Writer&)>;

namespace impl {

struct MetricsSource final {
    std::string prefix_path;
    std::vector<std::string> path_segments;
    ExtenderFunc extender;

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

    /// Visits all the metrics and calls `out.HandleMetric` for each metric.
    void VisitMetrics(BaseFormatBuilder& out, const Request& request = {}) const;

    /// @cond
    /// Must be called from StatisticsStorage only. Don't call it from user components.
    void StopRegisteringExtenders();
    /// @endcond

    /// @brief Add a writer function @b func. Note that `func` is called concurrently with
    /// other code, so it should be thread-safe.
    ///
    /// Registration runs after construction via @ref ResourceScopeStorage::AfterConstruction.
    /// Unregister runs in @ref ResourceScopeStorage::BeforeDestruction.
    ///
    /// In a component constructor prefer @ref RegisterWriterScope with @ref components::ComponentContext.
    ///
    /// @param scopes storage that owns the writer lifetime. In a component constructor pass `context.Scopes()`
    /// or @ref components::GetResourceScopes.
    /// @param common_prefix prefix for the metric, for example "my.metric_name"
    /// @param func function that writes metrics to @ref utils::statistics::Writer
    /// @param add_labels common labels for the metric, for example {"database", "dbname"}
    void RegisterWriter(
        ResourceScopeStorage& scopes,
        std::string common_prefix,
        WriterFunc func,
        std::vector<Label> add_labels = {}
    );

    /// @overload
    ///
    /// Store the returned @ref Entry as a member.
    Entry RegisterWriter(std::string common_prefix, WriterFunc func, std::vector<Label> add_labels = {});

    /// @deprecated Use RegisterWriter instead.
    Entry RegisterExtender(std::string prefix, ExtenderFunc func);

    void UnregisterExtender(impl::StorageIterator iterator, impl::UnregisteringKind kind) noexcept;

private:
    Entry DoRegisterExtender(impl::MetricsSource&& source);

    std::atomic<bool> may_register_extenders_;
    impl::StorageData metrics_sources_;
    mutable engine::SharedMutex mutex_;
};

/// @deprecated Use @ref Storage::RegisterWriter that takes @ref ResourceScopeStorage.
[[deprecated("Use Storage::RegisterWriter instead")]]
void RegisterWriterScope(
    ResourceScopeStorage& scope_storage,
    Storage& storage,
    std::string common_prefix,
    WriterFunc func,
    std::vector<Label> add_labels = {}
);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
