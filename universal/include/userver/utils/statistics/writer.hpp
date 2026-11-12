#pragma once

/// @file userver/utils/statistics/writer.hpp
/// @brief @copybrief utils::statistics::Writer
/// @ingroup userver_universal

#include <atomic>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <type_traits>

#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/statistics/histogram_view.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/rate.hpp>

// TODO(USERVER-1855) remove extra includes
#include <fmt/format.h>
#include <userver/utils/function_ref.hpp>
#include <userver/utils/statistics/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Writer;
class MetricValue;

namespace impl {
struct WriterState;
}  // namespace impl

/// @brief Returns true, if the `Metric` could be written by @ref utils::statistics::Writer.
///
/// In other words, checks that the `DumpMetric` for the `Metric` is provided or
/// that the metric could be written without providing one.
template <class Metric>
concept HasWriterSupport = std::is_arithmetic_v<Metric> || requires(Writer& writer, const Metric& metric) {
    DumpMetric(writer, metric);
};

/// @ingroup userver_universal
///
/// @brief Class for writing metrics that is provided by utils::statistics::Storage.
///
/// Usage is quite straightforward:
///
/// @snippet core/src/utils/statistics/storage_test.cpp  Writer basic sample
///
/// The above sample would produce the following metrics:
///
/// @snippet core/src/utils/statistics/storage_test.cpp  metrics pretty
///
/// The Writer can be customized for writing custom metric types by providing a
/// `void DumpMetric(utils::statistics::Writer& writer, const CustomMetric& value)`
/// function:
///
/// @snippet universal/src/utils/statistics/writer_test.cpp  DumpMetric basic
///
/// DumpMetric functions nest well, labels are propagated to the nested
/// DumpMetric:
///
/// @snippet universal/src/utils/statistics/writer_test.cpp  DumpMetric nested
///
/// To use the above writers register the metric writer in
/// @ref utils::statistics::Storage component:
///
/// @snippet core/src/utils/statistics/storage_test.cpp  DumpMetric RegisterWriter
///
/// The above metrics in Graphite format would look like:
/// @snippet core/src/utils/statistics/storage_test.cpp  metrics graphite
///
/// The Writer is usable by the utils::statistics::MetricTag. For example, for
/// the following structure:
/// @snippet samples/tcp_full_duplex_service/main.cpp  TCP sample - Stats definition
///
/// The DumpMetric function may look like:
/// @snippet samples/tcp_full_duplex_service/main.cpp  TCP sample - Stats tag
///
/// A writer and its children share mutable path and label state. Destroy a child before writing through its parent
/// or creating a sibling. Use a nested block to delimit the child's lifetime when necessary. Writers are neither
/// copyable nor movable; store a child in a local variable before passing it as `Writer&`.
///
/// Emit each metric, identified by its path and labels, once per collection. Writing a metric again does not replace
/// its previously exported value. Compute the final value before exporting it.
///
/// For introduction to metrics see @ref scripts/docs/en/userver/metrics.md
///
/// For metrics testing in testsuite see @ref TESTSUITE_METRICS_TESTING "Testsuite - Metrics".
/// For metrics testing in unit tests see @ref utils::statistics::Snapshot.
class Writer final {
public:
    /// Path parts delimiter. In other words, writer["a"]["b"] becomes "a.b"
    static constexpr char kDelimiter = '.';

    Writer() = delete;
    Writer(Writer&& other) = delete;
    Writer(const Writer&) = delete;
    Writer& operator=(Writer&&) = delete;
    Writer& operator=(const Writer&) = delete;

    ~Writer();

    /// Returns a child writer with `path` appended, separated by a dot. Dots in `path` are separators too:
    /// `writer["a.b"]` and `writer["a"]["b"]` produce the same path. Output formats handle escaping; do not replace
    /// dots with underscores manually. The path's contents are copied during the call.
    /// See @ref utils::statistics::Writer for child lifetime restrictions.
    [[nodiscard]] Writer operator[](std::string_view path) &;

    /// @overload
    [[nodiscard]] Writer operator[](std::string_view path) &&;

    /// Write metric value to metrics builder via using DumpMetric
    /// function.
    template <class T>
    void operator=(const T& value) {
        if constexpr (std::is_arithmetic_v<T> || std::is_same_v<std::decay_t<T>, Rate> ||
                      std::is_same_v<std::decay_t<T>, HistogramView> || std::is_same_v<std::decay_t<T>, MetricValue>)
        {
            Write(value);
        } else {
            if (state_) {
                static_assert(
                    HasWriterSupport<T>,
                    "Cast the metric to an arithmetic type or provide a "
                    "`void DumpMetric(utils::statistics::Writer& writer, "
                    "const Metric& value)` function for the `Metric` type"
                );
                DumpMetric(*this, value);
            }
        }
    }

    /// Writes a metric value with additional labels. Labels are inherited by all metrics exported by the value's
    /// `DumpMetric`. Label strings are borrowed and must remain valid until this synchronous call returns, including
    /// any nested exports.
    template <class T>
    void ValueWithLabels(const T& value, LabelsSpan labels) {
        auto new_writer = MakeChild();
        new_writer.AppendLabelsSpan(labels);
        new_writer = value;
    }

    /// @overload
    template <class T>
    void ValueWithLabels(const T& value, std::initializer_list<LabelView> il) {
        ValueWithLabels(value, LabelsSpan{il});
    }

    /// @overload
    template <class T>
    void ValueWithLabels(const T& value, const LabelView& label) {
        ValueWithLabels(value, LabelsSpan{&label, &label + 1});
    }

    /// @cond
    /// func must be called even for filtered out Writers, e.g. to always collect
    /// totals
    template <class Func>
    void WithLabels(utils::impl::InternalTag, LabelsSpan labels, Func func) {
        auto new_writer = MakeChild();
        new_writer.AppendLabelsSpan(labels);
        func(new_writer);
    }

    template <class Func>
    void WithLabels(utils::impl::InternalTag, std::initializer_list<LabelView> il, Func func) {
        WithLabels(utils::impl::InternalTag{}, LabelsSpan{il}, func);
    }

    template <class Func>
    void WithLabels(utils::impl::InternalTag, const LabelView& label, Func func) {
        WithLabels(utils::impl::InternalTag{}, LabelsSpan{&label, &label + 1}, func);
    }
    /// @endcond

    /// Returns true if this writer would actually write data. Returns false if
    /// the data is not required by request and metrics construction could be
    /// skipped.
    explicit operator bool() const noexcept { return !!state_; }

    /// @cond
    explicit Writer(impl::WriterState* state) noexcept;
    explicit Writer(impl::WriterState& state, LabelsSpan labels);
    /// @endcond

private:
    using ULongLong = unsigned long long;

    using PathSizeType = std::uint16_t;
    using LabelsSizeType = std::uint8_t;

    void Write(unsigned long long value);
    void Write(long long value);
    void Write(double value);
    void Write(Rate value);
    void Write(HistogramView value);
    void Write(MetricValue value);

    void Write(float value) { Write(static_cast<double>(value)); }

    void Write(unsigned long value) { Write(static_cast<ULongLong>(value)); }
    void Write(long value) { Write(static_cast<long long>(value)); }
    void Write(unsigned int value) { Write(static_cast<long long>(value)); }
    void Write(int value) { Write(static_cast<long long>(value)); }
    void Write(unsigned short value) { Write(static_cast<long long>(value)); }
    void Write(short value) { Write(static_cast<long long>(value)); }

    Writer MakeChild();

    struct MoveTag {};
    Writer(Writer& other, MoveTag) noexcept;

    Writer MoveOut() noexcept { return Writer{*this, MoveTag{}}; }

    void ResetState() noexcept;
    void ValidateUsage();

    void AppendPath(std::string_view path);
    void AppendLabelsSpan(LabelsSpan labels);

    impl::WriterState* state_;
    const PathSizeType initial_path_size_;
    PathSizeType current_path_size_;
    const LabelsSizeType initial_labels_size_;
    LabelsSizeType current_labels_size_;
};

template <class Metric>
void DumpMetric(Writer& writer, const std::atomic<Metric>& m) {
    static_assert(std::atomic<Metric>::is_always_lock_free, "std::atomic misuse");
    writer = m.load();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
