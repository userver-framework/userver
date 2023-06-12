#pragma once

/// @file userver/utils/statistics/labels.hpp
/// @brief Owning and non owning labels representations.

#include <string>
#include <string_view>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Label;

/// @brief Non owning label name+value storage.
class LabelView final {
 public:
  LabelView() = default;
  LabelView(Label&& label) = delete;
  explicit LabelView(const Label& label) noexcept;
  LabelView(std::string_view name, std::string_view value) noexcept
      : name_(name), value_(value) {}

  explicit operator bool() const { return !name_.empty(); }

  std::string_view Name() const { return name_; }
  std::string_view Value() const { return value_; }

 private:
  std::string_view name_;
  std::string_view value_;
};

bool operator<(const LabelView& x, const LabelView& y) noexcept;
bool operator==(const LabelView& x, const LabelView& y) noexcept;

/// @brief Label name+value storage.
class Label final {
 public:
  Label() = default;
  explicit Label(LabelView view);
  Label(std::string name, std::string value);

  explicit operator bool() const { return !name_.empty(); }
  explicit operator LabelView() const { return {name_, value_}; }

  const std::string& Name() const { return name_; }
  const std::string& Value() const& { return value_; }
  std::string& Value() & { return value_; }
  std::string&& Value() && { return std::move(value_); }

 private:
  std::string name_;
  std::string value_;
};

bool operator<(const Label& x, const Label& y) noexcept;
bool operator==(const Label& x, const Label& y) noexcept;

/// @brief View over a continuous range of LabelView.
class LabelsSpan final {
 public:
  using iterator = const LabelView*;
  using const_iterator = const LabelView*;

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

}  // namespace utils::statistics

USERVER_NAMESPACE_END
