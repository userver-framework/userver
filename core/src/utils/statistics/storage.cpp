#include <userver/utils/statistics/storage.hpp>

#include <utility>

#include <boost/container/small_vector.hpp>

#include <userver/formats/common/utils.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>
#include <utils/statistics/value_builder_helpers.hpp>

#include <utils/statistics/entry_impl.hpp>
#include <utils/statistics/visitation.hpp>
#include <utils/statistics/writer_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

const std::string kVersionField = "$version";
constexpr int kVersion = 2;

class FakeFormatBuilder final : public BaseFormatBuilder {
public:
    void HandleMetric(std::string_view, LabelsSpan, const MetricValue&) override {}
};

void WriteWithFakeFormat(impl::MetricsSource& source)
{
    static constexpr std::string_view fake_prefix = "fake_prefix";
    if (source.writer) {
        FakeFormatBuilder builder;
        const Request request;
        impl::WriterState state{builder, request, {}, {}};
        auto writer = Writer{&state}[fake_prefix];
        source.writer(writer);
    }
    if (source.extender) {
        source.extender(StatisticsRequest{});
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

// During the `Entry::Unregister` call or destruction of `Entry`, all variables
// used by the writer or extender callback must be valid (must not be
// destroyed). A common cause of crashes in this place: there is no manual call
// to `Unregister`. In this case, check the lifetime of the data used by the
// callback.
[[maybe_unused]] void CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnregistering(impl::MetricsSource& source
) noexcept {
    try {
        WriteWithFakeFormat(source);
    } catch (const std::exception& e) {
        utils::AbortWithStacktrace(fmt::format(
            "Unhandled exception while statistics holder {} is unregistering automatically: {}",
            source.prefix_path,
            e.what()
        ));
    }
}

}  // namespace

Storage::Storage()
    : may_register_extenders_(true)
{}

formats::json::Value Storage::GetAsJson() const {
    formats::json::ValueBuilder result;
    result[kVersionField] = kVersion;

    const std::shared_lock lock(mutex_);

    for (const auto& entry : metrics_sources_) {
        if (entry.writer) {
            continue;
        }

        LOG_DEBUG() << "Getting statistics for prefix=" << entry.prefix_path;
        SetSubField(result, std::vector(entry.path_segments), entry.extender(StatisticsRequest{}));
    }

    return result.ExtractValue();
}

void Storage::VisitMetrics(BaseFormatBuilder& out, const Request& request) const {
    {
        impl::WriterState state{out, request, {}, {}};
        for (const auto& [name, value] : request.add_labels) {
            state.add_labels.emplace_back(name, value);
        }

        boost::container::small_vector<LabelView, 16> labels_vector;

        const std::shared_lock lock(mutex_);
        for (const auto& entry : metrics_sources_) {
            if (!entry.writer) {
                continue;
            }

            labels_vector.clear();
            labels_vector.reserve(entry.writer_labels.size());
            for (const auto& l : entry.writer_labels) {
                labels_vector.emplace_back(l);
            }

            try {
                auto writer =
                    (entry.prefix_path.empty()
                         ? Writer{state, labels_vector}
                         : Writer{state, labels_vector}[entry.prefix_path]);
                if (writer) {
                    LOG_DEBUG() << "Getting statistics for prefix=" << entry.prefix_path;
                    entry.writer(writer);
                }
            } catch (const std::exception& e) {
                UASSERT_MSG(
                    false,
                    fmt::format("Failed to write metrics for prefix '{}': {}", entry.prefix_path, e.what())
                );
                LOG_ERROR() << "Failed to write metrics for prefix '" << entry.prefix_path << "': " << e;
            }
        }
    }

    statistics::VisitMetrics(out, GetAsJson(), request);
}

void Storage::StopRegisteringExtenders() { may_register_extenders_ = false; }

Entry Storage::RegisterWriter(std::string prefix, WriterFunc func, std::vector<Label> add_labels) {
    return DoRegisterExtender(impl::MetricsSource{std::move(prefix), {}, {}, std::move(func), std::move(add_labels)});
}

Entry Storage::RegisterExtender(std::string prefix, ExtenderFunc func) {
    auto prefix_split = formats::common::SplitPathString(prefix);
    return DoRegisterExtender(impl::MetricsSource{std::move(prefix), std::move(prefix_split), std::move(func), {}, {}});
}

Entry Storage::DoRegisterExtender(impl::MetricsSource&& source) {
    UASSERT_MSG(
        may_register_extenders_.load(),
        "You may not register statistics extender outside of component "
        "constructors"
    );

    const std::lock_guard lock(mutex_);
    if constexpr (impl::kCheckSubscriptionUB) {
        CheckDataUsedByCallbackIsValidBeforeRegistering(source);
    }
    const auto res = metrics_sources_.insert(metrics_sources_.end(), std::move(source));
    return Entry(Entry::Impl{this, res});
}

void Storage::UnregisterExtender(impl::StorageIterator iterator, [[maybe_unused]] impl::UnregisteringKind kind)
    noexcept {
    const std::lock_guard lock(mutex_);
    if constexpr (impl::kCheckSubscriptionUB) {
        if (kind == impl::UnregisteringKind::kAutomatic) {
            // fake writer and extender call to check
            CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnregistering(*iterator);
        }
    }
    metrics_sources_.erase(iterator);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
