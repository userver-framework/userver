#pragma once

/// @file userver/utils/statistics/metric_tag_impl.hpp
/// @brief Implementation details of @ref utils::statistics::MetricTag.

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

template <typename Metric>
void ResetMetric(std::atomic<Metric>& m) {
    static_assert(std::atomic<Metric>::is_always_lock_free, "std::atomic misuse");
    m = Metric();
}

template <typename Metric>
concept HasResetMetric = requires(Metric& m) { ResetMetric(m); };

class MetricWrapperBase {
public:
    MetricWrapperBase& operator=(MetricWrapperBase&&) = delete;
    virtual ~MetricWrapperBase();

    virtual void DumpToWriter(utils::statistics::Writer& writer) = 0;

    virtual void Reset() = 0;
};

template <typename Metric>
class MetricWrapper final : public MetricWrapperBase {
    static_assert(
        utils::statistics::HasWriterSupport<Metric>,
        "Provide a `void DumpMetric(utils::statistics::Writer&, const Metric&)`"
        "function in the namespace of `Metric`."
    );

    static_assert(!std::is_arithmetic_v<Metric>, "Type is not atomic, use std::atomic<T> instead");

public:
    template <typename... Args>
    explicit MetricWrapper(std::in_place_t, const Args&... args)
        : data_(args...)
    {}

    void DumpToWriter(Writer& writer) override { writer = data_; }

    void Reset() override {
        if constexpr (HasResetMetric<Metric>) {
            ResetMetric(data_);
        }
    }

    Metric& Get() { return data_; }

private:
    Metric data_;
};

using MetricFactory = std::function<std::unique_ptr<MetricWrapperBase>()>;

template <typename Metric, typename... Args>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
MetricFactory MakeMetricFactory(Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
        static_assert(
            std::is_default_constructible_v<Metric>,
            "Metric type is not default-constructible. You can pass "
            "additional args to MetricTag to forward them to Metric."
        );
    } else {
        static_assert(
            std::is_constructible_v<Metric, const Args&...>,
            "Metric type is not constructible from the given args"
        );
        static_assert(
            (true && ... && std::is_copy_constructible_v<std::decay_t<Args>>),
            "Metric args must be copy-constructible"
        );
    }

    return [args...] { return std::make_unique<MetricWrapper<Metric>>(std::in_place, args...); };
}

struct MetricKey final {
    std::type_index idx;
    std::string path;

    bool operator==(const MetricKey& other) const noexcept { return idx == other.idx && path == other.path; }
};

void RegisterMetricInfo(const MetricKey& key, MetricFactory factory);

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
