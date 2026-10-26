#pragma once

/// @file userver/utils/statistics/testing.hpp
/// @brief Utilities for analyzing emitted metrics in unit tests

#include <iosfwd>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/metric_value.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {
struct SnapshotData;
}  // namespace impl

/// @brief Thrown by @ref utils::statistics::Snapshot queries on unexpected metrics states.
class MetricQueryError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// @brief Thrown when metrics written into a snapshot are invalid.
///
/// @see utils::statistics::DuplicateMetricError
/// @see utils::statistics::DuplicateLabelError
class InvalidMetricError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// @brief Thrown when a snapshot contains two metrics with the same path and labels.
class DuplicateMetricError final : public InvalidMetricError {
public:
    using InvalidMetricError::InvalidMetricError;
};

/// @brief Thrown when one metric lists the same label name more than once.
/// The values may differ.
class DuplicateLabelError final : public InvalidMetricError {
public:
    using InvalidMetricError::InvalidMetricError;
};

/// @brief A non-owning view of a single metric: its path, labels and value.
///
/// The referenced path, labels and histogram data must remain alive while the view is used.
struct MetricView final {
    /// Dot-separated metric path, e.g. `"http.requests"`. May be empty for a root metric.
    std::string_view path;
    /// Non-owning view of the metric's labels.
    LabelsSpan labels;
    /// Metric value. Numeric values are stored by value; @ref utils::statistics::HistogramView refers to external data.
    MetricValue value;
};

/// @brief Writes @a metric through @a writer.
void DumpMetric(Writer& writer, const MetricView& metric);

/// @brief Filters metrics collected by @ref utils::statistics::Snapshot constructors.
struct SnapshotBuildOptions {
    /// Keep metrics whose full path starts with this prefix; do not prepend it to emitted paths.
    /// When @ref utils::statistics::SnapshotBuildOptions::sliced is `true`, a nonempty prefix must match the whole path
    /// or end at a dot-separated segment boundary: `"api"` matches `"api"` and `"api.requests"`, but not `"apis"`.
    /// `std::nullopt` keeps all paths. An explicitly empty string is rejected with `std::invalid_argument`.
    /// When @ref utils::statistics::SnapshotBuildOptions::sliced is `true`, a trailing `.` is also rejected with
    /// `std::invalid_argument`.
    /// When @ref utils::statistics::SnapshotBuildOptions::sliced is `false`, use raw string prefix matching:
    /// `"api"` also matches `"apis"`.
    std::optional<std::string> require_prefix{};

    /// Keep metrics containing all these labels; additional labels are allowed.
    std::vector<Label> require_labels{};

    /// Remove @ref utils::statistics::SnapshotBuildOptions::require_prefix and
    /// @ref utils::statistics::SnapshotBuildOptions::require_labels from selected metrics before storing them.
    /// Also remove one separating `.` after the prefix.
    /// An exact prefix match becomes an empty path. A path equal to the prefix followed by just `.` is rejected
    /// with `std::invalid_argument`: for example, `"api."` sliced by `"api"` would become indistinguishable from
    /// the exact path `"api"`. Empty inner segments, such as in `"api..requests"`, remain allowed.
    /// If `false`, use raw string prefix filtering and preserve full paths and all labels.
    bool sliced{true};
};

/// @brief Prefix and labels applied to every metric in @ref utils::statistics::Snapshot::FromJson and
/// @ref utils::statistics::Snapshot::FromLayeredJson.
struct SnapshotLoadOptions {
    /// Prepended to every path from the document. A dot separates it from the path.
    std::string add_prefix{};

    /// Added to every metric. A label name that the metric already has throws
    /// @ref utils::statistics::DuplicateLabelError.
    std::vector<Label> add_labels{};
};

/// @brief A snapshot of metrics dumped via @ref utils::statistics::Writer.
///
/// @anchor statistics_snapshot_view_lifetime
/// ## Lifetime of returned views
/// Returned @ref utils::statistics::MetricView and @ref utils::statistics::MetricValue objects refer to snapshot data,
/// not to the vector returned by @ref utils::statistics::Snapshot::AllMetrics.
///
/// - Copy construction/assignment preserves source views without extending their lifetime.
/// - Move construction/assignment preserves source views, tying their lifetime to the destination instead.
/// - Assignment invalidates the destination's previous views; destruction invalidates views still tied to that
///   snapshot. Existing copies do not prevent invalidation.
class Snapshot final {
public:
    /// @brief Create a snapshot by dumping @a metric via @ref utils::statistics::Writer.
    ///
    /// @a metric is any type with @ref utils::statistics::Writer support. For simple metrics prefer
    /// a type with `DumpMetric` and pass it directly:
    /// @snippet universal/src/utils/statistics/testing_test.cpp  metrics Snapshot DumpMetric sample
    ///
    /// @ref utils::statistics::Storage also works — passing Storage is still
    /// a common usage when writers are registered there:
    /// @snippet core/src/utils/statistics/testing_test.cpp  metrics Snapshot sample
    ///
    /// Unlike production, a metric with an empty full path is allowed.
    /// @param options Filters and slicing mode; see @ref utils::statistics::SnapshotBuildOptions.
    /// By default, the required prefix and labels are removed on collection.
    /// @throws utils::statistics::InvalidMetricError if two metrics have the same path and labels,
    /// or one metric lists the same label name more than once.
    /// @throws std::invalid_argument if @ref utils::statistics::SnapshotBuildOptions::require_prefix contains an empty
    /// string, or, with slicing enabled, ends with `.`, or a selected path becomes ambiguous after removing the prefix
    /// and `.`.
    /// @throws std::exception if the @ref utils::statistics::Writer dump throws.
    explicit Snapshot(const HasWriterSupport auto& metric, const SnapshotBuildOptions& options = {})
        : Snapshot([&metric](Writer& writer) { writer = metric; }, options)
    {}

    /// @brief Create a snapshot by invoking @a writer.
    ///
    /// @ref utils::statistics::SnapshotBuildOptions::require_prefix is not prepended to paths emitted by @a writer;
    /// it filters them according to @ref utils::statistics::SnapshotBuildOptions::sliced.
    /// Unlike production, a metric with an empty full path is allowed.
    /// @param options Filters and slicing mode; see @ref utils::statistics::SnapshotBuildOptions.
    /// By default, the required prefix and labels are removed on collection.
    /// @throws utils::statistics::InvalidMetricError if two metrics have the same path and labels,
    /// or one metric lists the same label name more than once.
    /// @throws std::invalid_argument if @ref utils::statistics::SnapshotBuildOptions::require_prefix contains an empty
    /// string, or, with slicing enabled, ends with `.`, or a selected path becomes ambiguous after removing the prefix
    /// and `.`.
    explicit Snapshot(WriterFuncRef writer, const SnapshotBuildOptions& options = {});

    /// @deprecated Use the constructor taking @ref utils::statistics::SnapshotBuildOptions instead.
    /// Uses slicing: @a prefix and @a require_labels are removed during collection.
    /// An empty @a prefix means no prefix filter.
    explicit Snapshot(const HasWriterSupport auto& metric, std::string prefix, std::vector<Label> require_labels = {})
        : Snapshot([&metric](Writer& writer) { writer = metric; }, std::move(prefix), std::move(require_labels))
    {}

    /// @deprecated Use the constructor taking @ref utils::statistics::SnapshotBuildOptions instead.
    /// Uses slicing: @a prefix and @a require_labels are removed during collection.
    /// An empty @a prefix means no prefix filter.
    explicit Snapshot(WriterFuncRef writer, std::string prefix, std::vector<Label> require_labels = {});

    /// @name Copying and moving
    /// @see statistics_snapshot_view_lifetime
    /// @{
    Snapshot(const Snapshot& other) = default;
    Snapshot(Snapshot&& other) noexcept = default;
    Snapshot& operator=(const Snapshot& other) = default;
    Snapshot& operator=(Snapshot&& other) noexcept = default;
    /// @}

    /// @brief Load metrics from the JSON object produced by @ref utils::statistics::ToJsonFormat.
    ///
    /// - Top-level keys are metric paths.
    /// - Each value is an array of `{"type", "value", "labels"}`.
    /// - `GAUGE` is stored as `std::int64_t` when @ref formats::json::Value::IsInt64 returns `true`, otherwise as
    /// `double`.
    ///   This includes integral floating-point values such as `4.0`. Integers outside the `std::int64_t` range may
    ///   lose precision when converted to `double`; they are not inferred to be @ref utils::statistics::Rate.
    /// - `RATE` is @ref utils::statistics::Rate.
    /// - `HIST_RATE` is a histogram (`bounds`, `buckets`, `inf`).
    ///   `sum`, when present, is the total sum and is restored.
    ///   Sums of individual buckets are not part of the format and are not restored.
    /// @param options Prepended to every path and added to every metric.
    /// @snippet universal/src/utils/statistics/testing_test.cpp  metrics Snapshot FromJson
    /// @throws utils::statistics::InvalidMetricError if two metrics have the same path and labels,
    /// or one metric lists the same label name more than once.
    /// @throws formats::json::ExceptionWithPath if @a json does not match the format above.
    static Snapshot FromJson(const formats::json::Value& json, const SnapshotLoadOptions& options = {});

    /// @brief Load metrics from a layered JSON object.
    ///
    /// - Top-level keys are metric paths.
    /// - A nested key `"name = value"` (one space on each side of `=`) adds a label.
    ///   The value may be empty. An empty name, extra spaces, or a missing space around `=` are rejected.
    /// - Label keys can be nested several levels deep, one label per level.
    /// - A field named `labels` is rejected. That field belongs to @ref utils::statistics::Snapshot::FromJson.
    /// - A number is a `GAUGE`, stored as `std::int64_t` when @ref formats::json::Value::IsInt64 returns `true`,
    ///   otherwise as `double`, just as in @ref utils::statistics::Snapshot::FromJson. Large integers outside the
    ///   `std::int64_t` range may lose precision; use an explicit `RATE` object when that type is intended.
    /// - An object with `bounds` and `buckets` is a histogram. `inf` is required too.
    ///   `sum`, when present, is the total sum and is restored.
    ///   Sums of individual buckets are not part of the format and are not restored.
    /// - `{"type": "RATE", "value": N}` is also supported, and so are `"GAUGE"` and `"HIST_RATE"`.
    /// @snippet universal/src/utils/statistics/testing_test.cpp  metrics Snapshot EqualsNumerically
    /// @throws utils::statistics::InvalidMetricError if two metrics have the same path and labels,
    /// or one metric lists the same label name more than once.
    /// @throws formats::json::ExceptionWithPath if @a json does not match the format above.
    static Snapshot FromLayeredJson(const formats::json::Value& json, const SnapshotLoadOptions& options = {});

    /// @brief Returns all metrics collected in this snapshot.
    ///
    /// Each element is a @ref utils::statistics::MetricView; see @ref statistics_snapshot_view_lifetime.
    ///
    /// Returns stored paths and labels: when @ref utils::statistics::SnapshotBuildOptions::sliced was `true` during
    /// collection, @ref utils::statistics::SnapshotBuildOptions::require_prefix and
    /// @ref utils::statistics::SnapshotBuildOptions::require_labels have already been removed. Otherwise, full paths
    /// and all labels are preserved.
    ///
    /// **Order:** element order is not guaranteed; do not rely on it in tests or production checks.
    ///
    /// The returned vector supports building a filtered @ref utils::statistics::Snapshot through @ref
    /// utils::statistics::Snapshot(WriterFuncRef), for example:
    /// @snippet universal/src/utils/statistics/testing_test.cpp metrics Snapshot filter by iteration
    std::vector<MetricView> AllMetrics() const USERVER_IMPL_LIFETIME_BOUND;

    /// @brief Find a single metric by the given filter.
    ///
    /// @a path and @a require_labels refer to stored metrics, as returned by @ref
    /// utils::statistics::Snapshot::AllMetrics. When @ref utils::statistics::SnapshotBuildOptions::sliced is `true`, do
    /// not repeat the prefix or labels removed during collection. When it is `false`, use full paths and original
    /// labels. The constructor's filter is never reapplied to queries.
    /// @param path The path of the target metric.
    /// @param require_labels Labels that the target metric should have; additional labels are allowed.
    /// @returns The value of the single found metric.
    /// @see statistics_snapshot_view_lifetime
    /// @throws utils::statistics::MetricQueryError if none or multiple metrics are found.
    /// @snippet universal/src/utils/statistics/testing_test.cpp metrics Snapshot sliced queries
    MetricValue SingleMetric(std::string path, std::vector<Label> require_labels = {}) const
        USERVER_IMPL_LIFETIME_BOUND;

    /// @brief Find a single metric by the given filter.
    /// @param path The path of the target metric.
    /// @param require_labels Labels that the target metric should have; additional labels are allowed.
    /// @returns The value when one metric matches, or `std::nullopt` when none match.
    /// @see statistics_snapshot_view_lifetime
    /// @throws utils::statistics::MetricQueryError when more than one metric matches.
    /// @see utils::statistics::Snapshot::SingleMetric for sliced and unsliced query examples.
    std::optional<MetricValue> SingleMetricOptional(std::string path, std::vector<Label> require_labels = {}) const
        USERVER_IMPL_LIFETIME_BOUND;

    /// @brief Collected metrics compared with a reference.
    ///
    /// Stored paths, labels and values are compared with those of @a other. Order does not matter.
    /// Constructor filters are not retained: @ref utils::statistics::SnapshotBuildOptions::sliced affects equality
    /// through the stored paths and labels.
    ///
    /// `std::int64_t`, `double` and @ref utils::statistics::Rate compare as numbers: `4`, `4.0`
    /// and `Rate{4}` are equal. A snapshot loaded from JSON can therefore
    /// match metrics collected in a test.
    ///
    /// @ref utils::statistics::Snapshot::SingleMetric still returns the stored @ref utils::statistics::MetricValue. Its
    /// `operator==` keeps the type, so an integer metric is not equal to `4.0`.
    ///
    /// In a gtest, compare a snapshot with a reference via
    /// @ref utils::statistics::EqualsNumerically(const Snapshot& expected)
    /// rather than this method, so the failure message shows both snapshots.
    bool EqualsNumerically(const Snapshot& other) const;

    /// @brief Writes snapshot metrics through @a writer.
    ///
    /// Writes the stored paths and labels of @a snapshot as is. If @ref utils::statistics::SnapshotBuildOptions::sliced
    /// was `true` during collection, emits shortened paths and remaining labels; otherwise, emits full paths and all
    /// labels. Removed prefixes and labels are not restored.
    /// Makes `Snapshot` satisfy @ref utils::statistics::HasWriterSupport.
    friend void DumpMetric(Writer& writer, const Snapshot& snapshot);

private:
    friend void PrintTo(const Snapshot& data, std::ostream*);

    utils::SharedRef<const impl::SnapshotData> data_;
};

/// @brief Support for gtest diagnostics for @ref utils::statistics::Snapshot.
///
/// @warning Never check the printed value programmatically! The string is not
/// stable and may change, depending on the version of stdlib and userver.
///
/// **Valid usage:**
/// @code
/// EXPECT_EQ(..., ...) << testing::PrintToString(snapshot);
/// @endcode
///
/// **Invalid usage:**
/// @code
/// std::ostringstream stream;
/// PrintTo(snapshot, &stream);
/// EXPECT_EQ(stream.str(), ...);
/// @endcode
void PrintTo(const Snapshot& data, std::ostream*);

/// @brief Support for gtest diagnostics for @ref utils::statistics::MetricValue.
void PrintTo(MetricValue value, std::ostream*);

/// @brief Support for gtest diagnostics for @ref utils::statistics::Rate
void PrintTo(Rate value, std::ostream*);

/// @brief GTest matcher for @ref utils::statistics::Snapshot::EqualsNumerically.
///
/// @snippet universal/src/utils/statistics/testing_test.cpp  metrics Snapshot EqualsNumerically
::testing::Matcher<Snapshot> EqualsNumerically(const Snapshot& expected);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
