#include <userver/utils/statistics/storage.hpp>

#include <optional>
#include <utility>

#include <boost/container/small_vector.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/resource_scopes.hpp>
#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

class FakeFormatBuilder final : public BaseFormatBuilder {
public:
    void HandleMetric(std::string_view, LabelsSpan, const MetricValue&) override {}
};

void WriteWithFakeFormat(impl::MetricsSource& source) {
    if (source.writer) {
        FakeFormatBuilder builder;
        VisitMetrics(
            [&source](Writer& root_writer) {
                std::optional<Writer> prefixed;
                if (!source.prefix_path.empty()) {
                    prefixed.emplace(utils::LazyPrvalue([&] { return root_writer[source.prefix_path]; }));
                }
                Writer& writer = prefixed ? *prefixed : root_writer;
                source.writer(writer);
            },
            builder
        );
    }
}

[[maybe_unused]] void CheckDataUsedByCallbackIsValidBeforeRegistering(impl::MetricsSource& source) noexcept {
    try {
        WriteWithFakeFormat(source);
    } catch (const std::exception& e) {
        utils::AbortWithStacktrace(fmt::format(
            "Unhandled exception while statistics holder {} is registering: {}",
            source.prefix_path,
            e.what()
        ));
    }
}

// All data accessed by the writer must still be valid when its scope closes.
// Passing an incorrect ResourceScopeStorage& can unregister the writer after the captured data is destroyed.
// See scripts/docs/en/userver/resource_scopes.md, "Where to obtain scopes".
[[maybe_unused]] void CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnregistering(impl::MetricsSource& source
) noexcept {
    try {
        WriteWithFakeFormat(source);
    } catch (const std::exception& e) {
        utils::AbortWithStacktrace(fmt::format(
            "Unhandled exception while statistics holder {} is unregistering: {}",
            source.prefix_path,
            e.what()
        ));
    }
}

}  // namespace

Storage::Storage()
    : may_register_extenders_(true)
{}

void DumpMetric(Writer& writer, const Storage& storage) {
    boost::container::small_vector<LabelView, 16> labels_vector;

    const std::shared_lock lock(storage.mutex_);
    for (const auto& entry : storage.metrics_sources_) {
        if (!entry.writer) {
            continue;
        }

        labels_vector.clear();
        labels_vector.reserve(entry.writer_labels.size());
        for (const auto& l : entry.writer_labels) {
            labels_vector.emplace_back(l);
        }

        try {
            writer.WithLabels(utils::impl::InternalTag{}, labels_vector, [&entry](Writer& labeled) {
                std::optional<Writer> prefixed;
                if (!entry.prefix_path.empty()) {
                    prefixed.emplace(utils::LazyPrvalue([&] { return labeled[entry.prefix_path]; }));
                }
                Writer& writer = prefixed ? *prefixed : labeled;
                if (writer) {
                    LOG_DEBUG() << "Getting statistics for prefix=" << entry.prefix_path;
                    entry.writer(writer);
                }
            });
        } catch (const std::exception& e) {
            UASSERT_MSG(false, fmt::format("Failed to write metrics for prefix '{}': {}", entry.prefix_path, e.what()));
            LOG_ERROR() << "Failed to write metrics for prefix '" << entry.prefix_path << "': " << e;
        }
    }
}

void Storage::StopRegisteringExtenders() { may_register_extenders_ = false; }

void Storage::RegisterWriter(
    ResourceScopeStorage& scopes,
    std::string common_prefix,
    WriterFunc func,
    std::vector<Label> add_labels
) {
    scopes.Register([&storage = *this,
                     common_prefix = std::move(common_prefix),
                     func = std::move(func),
                     add_labels = std::move(add_labels)]() mutable {
        const auto iterator = storage.DoRegisterWriter(impl::MetricsSource{
            .prefix_path = std::move(common_prefix),
            .writer = std::move(func),
            .writer_labels = std::move(add_labels),
        });
        return utils::FastScopeGuard([&storage, iterator]() noexcept { storage.DoUnregisterWriter(iterator); });
    });
}

impl::StorageIterator Storage::DoRegisterWriter(impl::MetricsSource&& source) {
    UASSERT_MSG(
        may_register_extenders_.load(),
        "You may not register statistics extender outside of component "
        "constructors"
    );

    const std::lock_guard lock(mutex_);
    if constexpr (impl::kCheckSubscriptionUB) {
        CheckDataUsedByCallbackIsValidBeforeRegistering(source);
    }
    return metrics_sources_.insert(metrics_sources_.end(), std::move(source));
}

void Storage::DoUnregisterWriter(impl::StorageIterator iterator) noexcept {
    const std::lock_guard lock(mutex_);
    if constexpr (impl::kCheckSubscriptionUB) {
        // fake writer call to check
        CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnregistering(*iterator);
    }
    metrics_sources_.erase(iterator);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
