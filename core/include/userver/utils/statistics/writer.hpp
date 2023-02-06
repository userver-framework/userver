#pragma once

/// @file userver/utils/statistics/writer.hpp
/// @brief @copybrief utils::statistics::Writer

#include <atomic>
#include <string_view>
#include <type_traits>

#include <userver/utils/statistics/labels.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Writer;

namespace impl {

struct WriterState;

template <class Metric>
constexpr auto HasDumpMetricWriter() noexcept
    -> decltype(DumpMetric(std::declval<Writer&>(),
                           std::declval<const Metric&>()),
                std::true_type{}) {
  return {};
}

template <class Metric, class... Args>
constexpr auto HasDumpMetricWriter(Args...) noexcept {
  return std::is_arithmetic_v<Metric>;
}

}  // namespace impl

/// @brief Returns true, if the `Metric` could be written by
/// utils::statistics::Writer.
///
/// In other words, checks that the DumpMetric for the `Metric` is provided or
/// that the metric could be written without providing one.
template <class Metric>
inline constexpr bool kHasWriterSupport = impl::HasDumpMetricWriter<Metric>();

// clang-format off

/// @brief Class for writing metrics
///
/// The Writer could be customized by providing a
/// `void DumpMetric(utils::statistics::Writer& writer, const Metric& value)`
/// function:
///
/// @snippet core/src/utils/statistics/writer_test.cpp  DumpMetric basic
///
/// DumpMetric functions nest well, labels are propagated to the nested
/// DumpMetric:
///
/// @snippet core/src/utils/statistics/writer_test.cpp  DumpMetric nested
///
/// To use the above writers register the metric writer in
/// utils::statistics::Storage component:
///
/// @snippet core/src/utils/statistics/writer_test.cpp  DumpMetric RegisterWriter
///
/// The above metrics in Grphite format would look like:
/// @snippet core/src/utils/statistics/writer_test.cpp  metrics graphite
///
/// The Writer is usable by the utils::statistics::MetricTag. For example, for
/// the following structure:
/// @snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - Stats definition
///
/// The DumpMetric function may look like:
/// @snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - Stats tag
///
/// For information on metrics testing in testsuite refer to
/// @ref TESTSUITE_METRICS_TESTING "Testsuite - Metrics".
///
/// For metrics testing in unit-tests see utils::statistics::Snapshot.

// clang-format on

class Writer final {
 public:
  /// Path parts delimiter. In other words, writer["a"]["b"] becomes "a.b"
  static inline constexpr char kDelimiter = '.';

  Writer() = delete;
  Writer(Writer&& other) = delete;
  Writer(const Writer&) = delete;
  Writer& operator=(Writer&&) = delete;
  Writer& operator=(const Writer&) = delete;

  ~Writer();

  /// Returns a Writer with a ('.' + path) appended
  [[nodiscard]] Writer operator[](std::string_view path) &;

  /// Returns a Writer with a ('.' + path) appended
  [[nodiscard]] Writer operator[](std::string_view path) &&;

  /// Write metric value to metrics builder via using DumpMetric
  /// function.
  template <class T>
  void operator=(const T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
      Write(value);
    } else {
      if (state_) {
        static_assert(kHasWriterSupport<T>,
                      "Cast the metric to an arithmetic type or provide a "
                      "`void DumpMetric(utils::statistics::Writer& writer, "
                      "const Metric& value)` function for the `Metric` type");
        DumpMetric(*this, value);
      }
    }
  }

  /// Write metric value with labels to metrics builder
  template <class T>
  void ValueWithLabels(const T& value, LabelsSpan labels) {
    auto new_writer = MakeChild();
    new_writer.AppendLabelsSpan(labels);
    new_writer = value;
  }

  /// Write metric value with labels to metrics builder
  template <class T>
  void ValueWithLabels(const T& value, std::initializer_list<LabelView> il) {
    ValueWithLabels(value, LabelsSpan{il});
  }

  /// Write metric value with label to metrics builder
  template <class T>
  void ValueWithLabels(const T& value, const LabelView& label) {
    ValueWithLabels(value, LabelsSpan{&label, &label + 1});
  }

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
