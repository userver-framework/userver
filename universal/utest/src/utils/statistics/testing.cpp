#include <userver/utils/statistics/testing.hpp>

#include <algorithm>
#include <iterator>
#include <optional>
#include <ostream>
#include <ranges>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/container/flat_set.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/statistics/fmt.hpp>
#include <userver/utils/statistics/histogram_aggregator.hpp>
#include <utils/statistics/testing_formats.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {
namespace {

struct FoundMetric final {
    std::string path;
    std::vector<Label> labels;
    MetricValue value;
};

std::string ToString(const FoundMetric& metric) {
    return fmt::format("{};{} {}", metric.path, fmt::join(metric.labels, ";"), metric.value);
}

struct SnapshotDataEntry final {
    boost::container::flat_set<Label> labels;
    std::vector<LabelView> label_views;
    MetricValue value;

    bool operator==(const SnapshotDataEntry& other) const {
        return labels == other.labels && value.EqualsNumerically(other.value);
    }
};

constexpr std::string_view kTestPrefix = "test";

std::string MakeRequestPrefix(std::string prefix) {
    if (prefix.empty()) {
        return std::string{kTestPrefix};
    }
    return fmt::format("{}.{}", kTestPrefix, prefix);
}

}  // namespace

namespace impl {

struct SnapshotData final {
    std::unordered_map<std::string, std::vector<SnapshotDataEntry>> metrics;
    // A HistogramView written by a metric writer refers to the writer's memory, which may be freed
    // right after the dump, so the snapshot owns its own copies.
    std::vector<HistogramAggregator> histogram_storage;
};

}  // namespace impl

namespace {

MetricValue StoreMetric(impl::SnapshotData& data, MetricValue value) {
    if (value.IsHistogram()) {
        const auto source = value.AsHistogram();
        auto& stored = data.histogram_storage.emplace_back(HistogramAggregator::CopyBounds{}, source);
        stored.Add(source);
        return MetricValue{stored.GetView()};
    }
    return value;
}

std::string_view SlicePath(std::string_view path, std::string_view prefix) {
    UASSERT(path.starts_with(prefix));
    path.remove_prefix(prefix.size());
    if (!prefix.empty() && !prefix.ends_with('.') && path.starts_with('.')) {
        path.remove_prefix(1);
    }
    return path;
}

class SnapshotVisitor final : public BaseFormatBuilder {
public:
    explicit SnapshotVisitor(impl::SnapshotData& data, const SnapshotBuildOptions& options)
        : data_(data),
          options_(options)
    {}

    void HandleMetric(std::string_view path, LabelsSpan labels, const MetricValue& value) override {
        path = SlicePath(path, kTestPrefix);
        if (options_.sliced && options_.require_prefix) {
            const auto& prefix = options_.require_prefix.value();
            if (path.size() > prefix.size() && path[prefix.size()] != '.') {
                return;
            }
            if (path.size() == prefix.size() + 1) {
                throw std::invalid_argument(fmt::format(
                    "Metric path '{}' becomes empty right after prefix '{}', which would make prefix matching "
                    "ambiguous: it would be indistinguishable from path '{}'. This looks like a malformed metric path.",
                    path,
                    prefix,
                    prefix
                ));
            }
            path = SlicePath(path, prefix);
        }
        labels_buffer_.clear();
        labels_buffer_.reserve(labels.size());
        for (const auto view : labels) {
            labels_buffer_.emplace_back(view);
        }
        std::ranges::sort(labels_buffer_);
        const auto same_name = [](const Label& lhs, const Label& rhs) { return lhs.Name() == rhs.Name(); };
        if (const auto duplicate = std::ranges::adjacent_find(labels_buffer_, same_name);
            duplicate != labels_buffer_.end())
        {
            throw DuplicateLabelError(fmt::format("Duplicate labels at {}: {}", path, *duplicate));
        }

        if (options_.sliced) {
            std::erase_if(labels_buffer_, [this](const Label& label) {
                return std::ranges::find(options_.require_labels, label) != options_.require_labels.end();
            });
        }
        auto labels_owned = boost::container::flat_set<Label>{
            boost::container::ordered_unique_range,
            std::make_move_iterator(labels_buffer_.begin()),
            std::make_move_iterator(labels_buffer_.end()),
        };
        auto label_views = utils::AsContainer<std::vector<LabelView>>(labels_owned);
        data_.metrics[std::string{path}].push_back(SnapshotDataEntry{
            .labels = std::move(labels_owned),
            .label_views = std::move(label_views),
            .value = StoreMetric(data_, value),
        });
    }

private:
    impl::SnapshotData& data_;
    const SnapshotBuildOptions& options_;
    std::vector<Label> labels_buffer_;
};

void SortMetricEntries(impl::SnapshotData& data) {
    for (auto& [path, entries] : data.metrics) {
        std::ranges::sort(entries, [](const SnapshotDataEntry& lhs, const SnapshotDataEntry& rhs) {
            return lhs.labels < rhs.labels;
        });
        const auto duplicate =
            std::ranges::adjacent_find(entries, [](const SnapshotDataEntry& lhs, const SnapshotDataEntry& rhs) {
                return lhs.labels == rhs.labels;
            });
        if (duplicate != entries.end()) {
            throw DuplicateMetricError(fmt::format(
                "Multiple metrics with the same path and labels: {};{}",
                path,
                fmt::join(duplicate->labels, ";")
            ));
        }
    }
}

utils::SharedRef<const impl::SnapshotData> BuildSnapshotData(
    WriterFuncRef writer_func,
    const SnapshotBuildOptions& options
) {
    if (options.require_prefix && options.require_prefix.value().empty()) {
        throw std::invalid_argument("Snapshot require_prefix must not be empty; use std::nullopt for no prefix filter");
    }
    if (options.sliced && options.require_prefix && options.require_prefix.value().ends_with('.')) {
        throw std::invalid_argument("Snapshot require_prefix must not end with '.' when sliced is true");
    }
    const auto request =
        Request::MakeWithPrefix(MakeRequestPrefix(options.require_prefix.value_or("")), {}, options.require_labels);
    auto data = utils::MakeSharedRef<impl::SnapshotData>();
    SnapshotVisitor visitor{*data, options};
    utils::statistics::VisitMetrics(
        [&writer_func](Writer& writer) {
            auto prefixed = writer[kTestPrefix];
            writer_func(prefixed);
        },
        visitor,
        request
    );
    SortMetricEntries(*data);
    return data;
}

std::optional<FoundMetric> GetSingleOptional(
    const impl::SnapshotData& data,
    const std::string& path,
    const std::vector<Label>& required_labels
) {
    std::optional<FoundMetric> found_metric;
    const auto metrics_it = data.metrics.find(path);
    if (metrics_it == data.metrics.end()) {
        return std::nullopt;
    }

    for (const auto& entry : metrics_it->second) {
        const bool matches = boost::algorithm::all_of(required_labels, [&entry = entry](const auto& needle) {
            return entry.labels.count(needle) != 0;
        });

        if (matches) {
            FoundMetric new_metric{
                .path = path,
                .labels = utils::AsContainer<std::vector<Label>>(entry.labels),
                .value = entry.value,
            };
            if (found_metric) {
                throw MetricQueryError(fmt::format(
                    "Multiple metrics found for request {};{}\n  {}\n  {}",
                    path,
                    fmt::join(required_labels, ";"),
                    ToString(*found_metric),
                    ToString(new_metric)
                ));
            }
            found_metric = std::move(new_metric);
        }
    }

    return found_metric;
}

template <typename Metric>
void WriteWithLoadOptions(Writer& writer, const Metric& metric, const SnapshotLoadOptions& options) {
    const auto views = utils::AsContainer<std::vector<
        LabelView>>(options.add_labels | std::views::transform([](const Label& label) { return LabelView{label}; }));
    const auto span = LabelsSpan{views};
    if (options.add_prefix.empty()) {
        writer.ValueWithLabels(metric, span);
    } else {
        writer[options.add_prefix].ValueWithLabels(metric, span);
    }
}

}  // namespace

void DumpMetric(Writer& writer, const MetricView& metric) {
    if (metric.path.empty()) {
        writer.ValueWithLabels(metric.value, metric.labels);
    } else {
        writer[metric.path].ValueWithLabels(metric.value, metric.labels);
    }
}

Snapshot::Snapshot(WriterFuncRef writer, const SnapshotBuildOptions& options)
    : data_(BuildSnapshotData(writer, options))
{}

Snapshot::Snapshot(WriterFuncRef writer, std::string prefix, std::vector<Label> require_labels)
    : Snapshot(
          writer,
          SnapshotBuildOptions{
              .require_prefix = prefix.empty() ? std::nullopt : std::make_optional(std::move(prefix)),
              .require_labels = std::move(require_labels),
          }
      )
{}

Snapshot Snapshot::FromJson(const formats::json::Value& json, const SnapshotLoadOptions& options) {
    return Snapshot([&json, &options](Writer& writer) { WriteWithLoadOptions(writer, impl::FlatJson{json}, options); });
}

Snapshot Snapshot::FromLayeredJson(const formats::json::Value& json, const SnapshotLoadOptions& options) {
    return Snapshot([&json, &options](Writer& writer) {
        WriteWithLoadOptions(writer, impl::LayeredJson{json}, options);
    });
}

MetricValue Snapshot::SingleMetric(std::string path, std::vector<Label> require_labels) const
    USERVER_IMPL_LIFETIME_BOUND {
    const auto result = statistics::GetSingleOptional(*data_, path, require_labels);
    if (!result) {
        throw MetricQueryError(fmt::format("No metric found for request {};{}", path, fmt::join(require_labels, ";")));
    }
    return result->value;
}

std::optional<MetricValue> Snapshot::SingleMetricOptional(std::string path, std::vector<Label> require_labels) const
    USERVER_IMPL_LIFETIME_BOUND {
    const auto result = statistics::GetSingleOptional(*data_, path, require_labels);
    return result ? std::make_optional(result->value) : std::nullopt;
}

bool Snapshot::EqualsNumerically(const Snapshot& other) const { return data_->metrics == other.data_->metrics; }

void DumpMetric(Writer& writer, const Snapshot& snapshot) {
    for (const auto& metric : snapshot.AllMetrics()) {
        DumpMetric(writer, metric);
    }
}

std::vector<MetricView> Snapshot::AllMetrics() const {
    std::vector<MetricView> metrics;
    for (const auto& [path, entries] : data_->metrics) {
        for (const auto& entry : entries) {
            metrics.push_back(MetricView{
                .path = path,
                .labels = LabelsSpan{entry.label_views},
                .value = entry.value,
            });
        }
    }

#ifndef NDEBUG
    // Randomize the order so tests do not become coupled to metric ordering.
    if (utils::RandRange(2) == 1) {
        std::ranges::reverse(metrics);
    }
#endif

    return metrics;
}

void PrintTo(const Snapshot& data, std::ostream* out) {
    UASSERT(out);
    *out << "{";
    for (const auto& metric : data.AllMetrics()) {
        *out << fmt::format("{};{} {}", metric.path, fmt::join(metric.labels, ";"), metric.value);
        *out << ";\n";
    }
    *out << "}";
}

void PrintTo(MetricValue value, std::ostream* out) {
    UASSERT(out);
    *out << fmt::to_string(value);
}

void PrintTo(Rate value, std::ostream* out) {
    UASSERT(out);
    *out << fmt::to_string(value.value);
}

namespace {

class NumericallyEqualMatcher final {
public:
    using is_gtest_matcher = void;

    explicit NumericallyEqualMatcher(const Snapshot& expected)
        : expected_(expected)
    {}

    bool MatchAndExplain(const Snapshot& actual, std::ostream*) const { return actual.EqualsNumerically(expected_); }

    void DescribeTo(std::ostream* os) const {
        *os << "numerically equals ";
        PrintTo(expected_, os);
    }

    void DescribeNegationTo(std::ostream* os) const {
        *os << "does not numerically equal ";
        PrintTo(expected_, os);
    }

private:
    Snapshot expected_;
};

}  // namespace

::testing::Matcher<Snapshot> EqualsNumerically(const Snapshot& expected) { return NumericallyEqualMatcher{expected}; }

}  // namespace utils::statistics

USERVER_NAMESPACE_END
