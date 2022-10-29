#pragma once

/// @file userver/utils/statistics/writer.hpp
/// @brief Utilities for efficient writing of metrics

#include <string_view>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Label;

/// @brief Non owning label name+value storage.
class LabelView {
 public:
  LabelView() = default;
  LabelView(Label&& label) = delete;
  explicit LabelView(const Label& label) noexcept;
  LabelView(std::string_view name, std::string_view value) noexcept
      : name_(name), value_(value) {}

  explicit operator bool() { return !name_.empty(); }

  std::string_view Name() const { return name_; }
  std::string_view Value() const { return value_; }

 private:
  std::string_view name_;
  std::string_view value_;
};

bool operator<(const LabelView& x, const LabelView& y) noexcept;
bool operator==(const LabelView& x, const LabelView& y) noexcept;

/// @brief View over a continious range of LabelView.
class LabelsSpan {
 public:
  LabelsSpan() = default;
  LabelsSpan(const LabelView* begin, const LabelView* end) noexcept;
  LabelsSpan(std::initializer_list<LabelView> il) noexcept
      : LabelsSpan(il.begin(), il.end()) {}

  template <class Container>
  explicit LabelsSpan(
      const Container& cont,
      std::enable_if_t<std::is_same_v<decltype(*(cont.data() + cont.size())),
                                      const LabelView&>>* = nullptr) noexcept
      : LabelsSpan(cont.data(), cont.data() + cont.size()) {}

  const LabelView* begin() const noexcept { return begin_; }
  const LabelView* end() const noexcept { return end_; }
  std::size_t size() const noexcept { return end_ - begin_; }
  bool empty() const noexcept { return end_ == begin_; }

 private:
  const LabelView* begin_{nullptr};
  const LabelView* end_{nullptr};
};

namespace impl {

struct WriterState;

template <class Writer, class Metric>
void DumpMetric(Writer&&, Metric&&) {
  static_assert(sizeof(Metric) == 0,
                "Cast the metric to an arithmetic type or provide a "
                "specialization of `void DumpMetric(utils::statistics::Writer "
                "writer, const Metric& value)` for the Metric type");
}

}  // namespace impl

/// @brief Class for writing metrics
class Writer final {
 public:
  /// Path parts delimiter. In other words, writer["a"]["b"] becomes "a.b"
  static inline constexpr char kDelimiter = '.';

  /// @cond
  Writer(impl::WriterState* state, std::string_view path, LabelsSpan labels);
  /// @endcond

  Writer(Writer&& other) noexcept;

  Writer(const Writer&) = delete;
  Writer& operator=(Writer&&) = delete;
  Writer& operator=(const Writer&) = delete;

  ~Writer();

  /// Returns a Writer with a ('.' + path) appended
  Writer operator[](std::string_view path);

  /// Write arithmetic metric value without labels to metrics builder
  template <class T>
  std::enable_if_t<std::is_arithmetic_v<T>> operator=(T value) {
    Write(value);
  }

  /// Write metric value without labels to metrics builder via custom DumpMetric
  /// function.
  template <class T>
  std::enable_if_t<!std::is_arithmetic_v<T>> operator=(const T& value) {
    if (state_) {
      using impl::DumpMetric;  // poison pill
      DumpMetric(Writer{state_, {}, {}}, value);
    }
  }

  /// Write metric value with labels to metrics builder
  template <class T>
  Writer&& ValueWithLabels(const T& value, LabelsSpan labels) {
    if constexpr (std::is_arithmetic_v<T>) {
      Writer{state_, {}, labels}.Write(value);
    } else {
      if (state_) {
        using impl::DumpMetric;  // poison pill
        DumpMetric(Writer{state_, {}, labels}, value);
      }
    }

    return std::move(*this);
  }

  /// Write metric value with labels to metrics builder
  template <class T>
  Writer&& ValueWithLabels(const T& value,
                           std::initializer_list<LabelView> il) {
    return ValueWithLabels(value, LabelsSpan{il});
  }

  /// Write metric value with label to metrics builder
  template <class T>
  Writer&& ValueWithLabels(const T& value, const LabelView& label) {
    return ValueWithLabels(value, LabelsSpan{&label, &label + 1});
  }

  /// Returns true if this writer would actually write data. Returns false if
  /// the data is not required by request and metrics construction could be
  /// skipped.
  explicit operator bool() const noexcept { return !!state_; }

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

  void ResetState() noexcept;
  void ValidateUsage();

  // In main constructor we need a constructor call that succeeds. In that case
  // even if the main constructor throws, the ~Writer() is called.
  struct MarkDestructionReady {};
  Writer(impl::WriterState* state, MarkDestructionReady) noexcept;

  impl::WriterState* state_;
  const PathSizeType initial_path_size_;
  PathSizeType current_path_size_;
  const LabelsSizeType initial_labels_size_;
  LabelsSizeType current_labels_size_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
