#pragma once

/// @file userver/utils/trivial_map.hpp
/// @brief Bidirectional map|sets over string literals or other trivial types.

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

constexpr bool HasUppercaseAscii(std::string_view value) noexcept {
  for (auto c : value) {
    if ('A' <= c && c <= 'Z') return true;
  }

  return false;
}

constexpr bool ICaseEqualLowercase(std::string_view lowercase,
                                   std::string_view y) noexcept {
  const auto size = lowercase.size();
  UASSERT(size == y.size());
  constexpr char kLowerToUpperMask = static_cast<char>(~unsigned{32});
  for (std::size_t i = 0; i < size; ++i) {
    const auto lowercase_c = lowercase[i];
    UASSERT(!('A' <= lowercase_c && lowercase_c <= 'Z'));
    if (lowercase_c != y[i]) {
      if (!('a' <= lowercase_c && lowercase_c <= 'z') ||
          (lowercase_c & kLowerToUpperMask) != y[i]) {
        return false;
      }
    }
  }

  return true;
}

struct Found final {
  constexpr explicit Found(std::size_t value) noexcept { UASSERT(value == 0); }

  constexpr explicit operator std::size_t() const noexcept { return 0; }
};

template <typename Key, typename Value, typename Enabled = void>
class SearchState final {
 public:
  constexpr explicit SearchState(Key key) noexcept
      : key_or_result_(std::in_place_index<0>, key) {}

  constexpr bool IsFound() const noexcept {
    return key_or_result_.index() != 0;
  }

  constexpr Key GetKey() const noexcept {
    UASSERT(!IsFound());
    return std::get<0>(key_or_result_);
  }

  constexpr void SetValue(Value value) noexcept {
    key_or_result_ = std::variant<Key, Value>(std::in_place_index<1>, value);
  }

  [[nodiscard]] constexpr std::optional<Value> Extract() noexcept {
    if (key_or_result_.index() == 1) {
      return std::get<1>(key_or_result_);
    } else {
      return std::nullopt;
    }
  }

 private:
  std::variant<Key, Value> key_or_result_;
};

inline constexpr std::size_t kInvalidSize =
    std::numeric_limits<std::size_t>::max();

template <typename Payload>
inline constexpr bool kFitsInStringOrPayload =
    sizeof(Payload) <= sizeof(const char*) &&
    (std::is_integral_v<Payload> || std::is_enum_v<Payload> ||
     std::is_same_v<Payload, Found>);

// A compacted std::variant<std::string_view, Payload>
template <typename Payload>
class StringOrPayload final {
 public:
  constexpr explicit StringOrPayload(std::string_view string) noexcept
      : data_or_payload_(string.data()), size_(string.size()) {
#if defined(__clang__)
    __builtin_assume(size_ != kInvalidSize);
#elif defined(__GNUC__)
    if (size_ == kInvalidSize) __builtin_unreachable();
#endif
  }

  constexpr explicit StringOrPayload(Payload payload) noexcept
      : data_or_payload_(payload), size_(kInvalidSize) {}

  constexpr bool HasPayload() const noexcept { return size_ == kInvalidSize; }

  constexpr std::string_view GetString() const noexcept {
    UASSERT(!HasPayload());
    return std::string_view{data_or_payload_.data, size_};
  }

  constexpr Payload GetPayload() const noexcept {
    UASSERT(HasPayload());
    return data_or_payload_.payload;
  }

 private:
  static_assert(kFitsInStringOrPayload<Payload>);

  union DataOrPayload {
    constexpr explicit DataOrPayload(const char* data) noexcept : data(data) {}

    constexpr explicit DataOrPayload(Payload payload) noexcept
        : payload(payload) {}

    const char* data{};
    Payload payload;
  };

  DataOrPayload data_or_payload_;
  std::size_t size_;
};

template <typename Value>
class SearchState<std::string_view, Value,
                  std::enable_if_t<kFitsInStringOrPayload<Value>>>
    final {
 public:
  constexpr explicit SearchState(std::string_view key) noexcept : state_(key) {}

  constexpr bool IsFound() const noexcept { return state_.HasPayload(); }

  constexpr std::string_view GetKey() const noexcept {
    return state_.GetString();
  }

  constexpr void SetValue(Value value) noexcept {
    state_ = StringOrPayload<Value>{value};
  }

  [[nodiscard]] constexpr std::optional<Value> Extract() noexcept {
    return IsFound() ? std::optional{state_.GetPayload()} : std::nullopt;
  }

 private:
  StringOrPayload<Value> state_;
};

template <typename Key>
class SearchState<Key, std::string_view,
                  std::enable_if_t<kFitsInStringOrPayload<Key>>>
    final {
 public:
  constexpr explicit SearchState(Key key) noexcept : state_(key) {}

  constexpr bool IsFound() const noexcept { return !state_.HasPayload(); }

  constexpr Key GetKey() const noexcept { return state_.GetPayload(); }

  constexpr void SetValue(std::string_view value) noexcept {
    state_ = StringOrPayload<Key>{value};
  }

  [[nodiscard]] constexpr std::optional<std::string_view> Extract() noexcept {
    return IsFound() ? std::optional{state_.GetString()} : std::nullopt;
  }

 private:
  StringOrPayload<Key> state_;
};

template <typename First, typename Second>
class SwitchByFirst final {
 public:
  constexpr explicit SwitchByFirst(First search) noexcept : state_(search) {}

  constexpr SwitchByFirst& Case(First first, Second second) noexcept {
    if (!state_.IsFound() && state_.GetKey() == first) {
      state_.SetValue(second);
    }
    return *this;
  }

  template <typename T, typename U = void>
  constexpr SwitchByFirst& Type() {
    return *this;
  }

  [[nodiscard]] constexpr std::optional<Second> Extract() noexcept {
    return state_.Extract();
  }

 private:
  SearchState<First, Second> state_;
};

template <typename First>
class SwitchByFirst<First, void> final {
 public:
  constexpr explicit SwitchByFirst(First search) noexcept : state_(search) {}

  constexpr SwitchByFirst& Case(First first) noexcept {
    if (!state_.IsFound() && state_.GetKey() == first) {
      state_.SetValue(Found{0});
    }
    return *this;
  }

  template <typename T, typename U = void>
  constexpr SwitchByFirst& Type() {
    return *this;
  }

  [[nodiscard]] constexpr bool Extract() noexcept { return state_.IsFound(); }

 private:
  SearchState<First, Found> state_;
};

template <typename Second>
class SwitchByFirstICase final {
 public:
  constexpr explicit SwitchByFirstICase(std::string_view search) noexcept
      : state_(search) {}

  constexpr SwitchByFirstICase& Case(std::string_view first,
                                     Second second) noexcept {
    UASSERT_MSG(!impl::HasUppercaseAscii(first),
                fmt::format("String literal '{}' in utils::Switch*::Case() "
                            "should be in lower case",
                            first));
    if (!state_.IsFound() && state_.GetKey().size() == first.size() &&
        impl::ICaseEqualLowercase(first, state_.GetKey())) {
      state_.SetValue(second);
    }
    return *this;
  }

  template <typename T, typename U>
  constexpr SwitchByFirstICase& Type() {
    return *this;
  }

  [[nodiscard]] constexpr std::optional<Second> Extract() noexcept {
    return state_.Extract();
  }

 private:
  SearchState<std::string_view, Second> state_;
};

template <>
class SwitchByFirstICase<void> final {
 public:
  constexpr explicit SwitchByFirstICase(std::string_view search) noexcept
      : state_(search) {}

  constexpr SwitchByFirstICase& Case(std::string_view first) noexcept {
    UASSERT_MSG(!impl::HasUppercaseAscii(first),
                fmt::format("String literal '{}' in utils::Switch*::Case() "
                            "should be in lower case",
                            first));
    if (!state_.IsFound() && state_.GetKey().size() == first.size() &&
        impl::ICaseEqualLowercase(first, state_.GetKey())) {
      state_.SetValue(Found{0});
    }
    return *this;
  }

  template <typename T, typename U>
  constexpr SwitchByFirstICase& Type() {
    return *this;
  }

  [[nodiscard]] constexpr bool Extract() const noexcept {
    return state_.IsFound();
  }

 private:
  SearchState<std::string_view, Found> state_;
};

template <typename First>
class SwitchBySecondICase final {
 public:
  constexpr explicit SwitchBySecondICase(std::string_view search) noexcept
      : state_(search) {}

  constexpr SwitchBySecondICase& Case(First first,
                                      std::string_view second) noexcept {
    UASSERT_MSG(!impl::HasUppercaseAscii(second),
                fmt::format("String literal '{}' in utils::Switch*::Case() "
                            "should be in lower case",
                            second));
    if (!state_.IsFound() && state_.GetKey().size() == second.size() &&
        impl::ICaseEqualLowercase(second, state_.GetKey())) {
      state_.SetValue(first);
    }
    return *this;
  }

  template <typename T, typename U>
  constexpr SwitchBySecondICase& Type() {
    return *this;
  }

  [[nodiscard]] constexpr std::optional<First> Extract() noexcept {
    return state_.Extract();
  }

 private:
  SearchState<std::string_view, First> state_;
};

template <typename First, typename Second>
class SwitchBySecond final {
 public:
  constexpr explicit SwitchBySecond(Second search) noexcept : state_(search) {}

  constexpr SwitchBySecond& Case(First first, Second second) noexcept {
    if (!state_.IsFound() && state_.GetKey() == second) {
      state_.SetValue(first);
    }
    return *this;
  }

  template <typename T, typename U>
  constexpr SwitchBySecond& Type() {
    return *this;
  }

  [[nodiscard]] constexpr std::optional<First> Extract() noexcept {
    return state_.Extract();
  }

 private:
  SearchState<Second, First> state_;
};

template <typename First, typename Second>
class SwitchTypesDetected final {
 public:
  using first_type = First;
  using second_type = Second;

  constexpr SwitchTypesDetected& Case(First, Second) noexcept { return *this; }
};

template <typename First>
class SwitchTypesDetected<First, void> final {
 public:
  using first_type = First;
  using second_type = void;

  constexpr SwitchTypesDetected& Case(First) noexcept { return *this; }
};

class SwitchTypesDetector final {
 public:
  constexpr SwitchTypesDetector& operator()() noexcept { return *this; }

  template <typename First, typename Second>
  constexpr auto Case(First, Second) noexcept {
    return Type<First, Second>();
  }

  template <typename First>
  constexpr auto Case(First) noexcept {
    return Type<First, void>();
  }

  template <typename First, typename Second = void>
  constexpr auto Type() noexcept {
    using first_type =
        std::conditional_t<std::is_convertible_v<First, std::string_view>,
                           std::string_view, First>;
    using second_type =
        std::conditional_t<std::is_convertible_v<Second, std::string_view>,
                           std::string_view, Second>;
    return SwitchTypesDetected<first_type, second_type>{};
  }
};

class CaseCounter final {
 public:
  template <typename First, typename Second>
  constexpr CaseCounter& Case(First, Second) noexcept {
    ++count_;
    return *this;
  }

  template <typename First>
  constexpr CaseCounter& Case(First) noexcept {
    ++count_;
    return *this;
  }

  template <typename T, typename U>
  constexpr CaseCounter& Type() {
    return *this;
  }

  [[nodiscard]] constexpr std::size_t Extract() const noexcept {
    return count_;
  }

 private:
  std::size_t count_{0};
};

class CaseDescriber final {
 public:
  template <typename First, typename Second>
  CaseDescriber& Case(First first, Second second) noexcept {
    if (!description_.empty()) {
      description_ += ", ";
    }

    description_ += fmt::format("('{}', '{}')", first, second);

    return *this;
  }

  template <typename T, typename U>
  constexpr CaseDescriber& Type() {
    return *this;
  }

  [[nodiscard]] std::string Extract() && noexcept {
    return std::move(description_);
  }

 private:
  std::string description_{};
};

class CaseFirstDescriber final {
 public:
  template <typename First>
  CaseFirstDescriber& Case(First first) noexcept {
    if (!description_.empty()) {
      description_ += ", ";
    }

    description_ += fmt::format("'{}'", first);

    return *this;
  }

  template <typename First, typename Second>
  CaseFirstDescriber& Case(First first, Second /*second*/) noexcept {
    return Case(first);
  }

  template <typename T, typename U>
  constexpr CaseFirstDescriber& Type() {
    return *this;
  }

  [[nodiscard]] std::string Extract() && noexcept {
    return std::move(description_);
  }

 private:
  std::string description_{};
};

class CaseSecondDescriber final {
 public:
  template <typename First, typename Second>
  CaseSecondDescriber& Case(First /*first*/, Second second) noexcept {
    if (!description_.empty()) {
      description_ += ", ";
    }

    description_ += fmt::format("'{}'", second);

    return *this;
  }

  template <typename T, typename U>
  constexpr CaseSecondDescriber& Type() {
    return *this;
  }

  [[nodiscard]] std::string Extract() && noexcept {
    return std::move(description_);
  }

 private:
  std::string description_{};
};

template <typename First, typename Second>
class CaseGetValuesByIndex final {
 public:
  explicit constexpr CaseGetValuesByIndex(std::size_t search_index)
      : index_(search_index + 1) {}

  constexpr CaseGetValuesByIndex& Case(First first, Second second) noexcept {
    if (index_ == 0) {
      return *this;
    }
    if (index_ == 1) {
      first_ = first;
      second_ = second;
    }
    --index_;

    return *this;
  }

  template <typename T, typename U>
  constexpr CaseGetValuesByIndex& Type() {
    return *this;
  }

  [[nodiscard]] constexpr First GetFirst() noexcept {
    return std::move(first_);
  }

  [[nodiscard]] constexpr Second GetSecond() noexcept {
    return std::move(second_);
  }

 private:
  std::size_t index_;
  First first_{};
  Second second_{};
};

template <typename First>
class CaseFirstIndexer final {
 public:
  constexpr explicit CaseFirstIndexer(First search_value) noexcept
      : state_(search_value) {}

  constexpr CaseFirstIndexer& Case(First first) noexcept {
    if (!state_.IsFound() && state_.GetKey() == first) {
      state_.SetValue(index_);
    }
    ++index_;
    return *this;
  }

  template <typename T, typename U = void>
  constexpr CaseFirstIndexer& Type() {
    return *this;
  }

  [[nodiscard]] constexpr std::optional<std::size_t> Extract() && noexcept {
    return state_.Extract();
  }

 private:
  SearchState<First, std::size_t> state_;
  std::size_t index_ = 0;
};

}  // namespace impl

/// @ingroup userver_universal userver_containers
///
/// @brief Bidirectional unordered map for trivial types, including string
/// literals; could be efficiently used as a unordered non-bidirectional map.
///
/// @snippet universal/src/utils/trivial_map_test.cpp  sample string bimap
///
/// utils::TrivialBiMap and utils::TrivialSet are known to outperform
/// std::unordered_map if:
/// * there's 32 or less elements in map/set
/// * or keys are string literals and all of them differ in length.
///
/// Implementation of string search is \b very efficient due to
/// modern compilers optimize it to a switch by input string
/// length and an integral comparison (rather than a std::memcmp call). In other
/// words, it usually takes O(1) to find the match in the map.
///
/// The same story with integral or enum mappings - compiler optimizes them
/// into a switch and it usually takes O(1) to find the match.
///
/// @snippet universal/src/utils/trivial_map_test.cpp  sample bidir bimap
///
/// Empty map:
///
/// @snippet universal/src/utils/trivial_map_test.cpp  sample empty bimap
///
/// For a single value Case statements see @ref utils::TrivialSet.
template <typename BuilderFunc>
class TrivialBiMap final {
  using TypesPair =
      std::invoke_result_t<const BuilderFunc&, impl::SwitchTypesDetector>;

 public:
  using First = typename TypesPair::first_type;
  using Second = typename TypesPair::second_type;

  struct value_type {
    First first;
    Second second;
  };

  /// Returns Second if T is convertible to First, otherwise returns Second
  /// type.
  template <class T>
  using MappedTypeFor =
      std::conditional_t<std::is_convertible_v<T, First>, Second, First>;

  constexpr TrivialBiMap(BuilderFunc&& func) noexcept : func_(std::move(func)) {
    static_assert(std::is_empty_v<BuilderFunc>,
                  "Mapping function should not capture variables");
    static_assert(std::is_trivially_copyable_v<First>,
                  "First type in Case must be trivially copyable");
    static_assert(!std::is_void_v<Second>,
                  "If second type in Case is missing, use "
                  "utils::TrivialSet instead of utils::TrivialBiMap");
    static_assert(std::is_trivially_copyable_v<Second>,
                  "Second type in Case must be trivially copyable");
  }

  constexpr std::optional<Second> TryFindByFirst(First value) const noexcept {
    return func_(
               [value]() { return impl::SwitchByFirst<First, Second>{value}; })
        .Extract();
  }

  constexpr std::optional<First> TryFindBySecond(Second value) const noexcept {
    return func_(
               [value]() { return impl::SwitchBySecond<First, Second>{value}; })
        .Extract();
  }

  template <class T>
  constexpr std::optional<MappedTypeFor<T>> TryFind(T value) const noexcept {
    static_assert(
        !std::is_convertible_v<T, First> || !std::is_convertible_v<T, Second>,
        "Ambiguous conversion, use TryFindByFirst/TryFindBySecond instead");

    if constexpr (std::is_convertible_v<T, First>) {
      return TryFindByFirst(value);
    } else {
      return TryFindBySecond(value);
    }
  }

  /// @brief Case insensitive search for value.
  ///
  /// For efficiency reasons, first parameter in Case() should be lower case
  /// string literal.
  constexpr std::optional<Second> TryFindICaseByFirst(
      std::string_view value) const noexcept {
    return func_([value]() { return impl::SwitchByFirstICase<Second>{value}; })
        .Extract();
  }

  /// @brief Case insensitive search for value.
  ///
  /// For efficiency reasons, second parameter in Case() should be lower case
  /// string literal.
  constexpr std::optional<First> TryFindICaseBySecond(
      std::string_view value) const noexcept {
    return func_([value]() { return impl::SwitchBySecondICase<First>{value}; })
        .Extract();
  }

  /// @brief Case insensitive search for value that calls either
  /// TryFindICaseBySecond or TryFindICaseByFirst.
  constexpr std::optional<MappedTypeFor<std::string_view>> TryFindICase(
      std::string_view value) const noexcept {
    static_assert(!std::is_convertible_v<std::string_view, First> ||
                      !std::is_convertible_v<std::string_view, Second>,
                  "Ambiguous conversion, use "
                  "TryFindICaseByFirst/TryFindICaseBySecond");

    if constexpr (std::is_convertible_v<std::string_view, First>) {
      return TryFindICaseByFirst(value);
    } else {
      return TryFindICaseBySecond(value);
    }
  }

  /// Returns count of Case's in mapping
  constexpr std::size_t size() const noexcept {
    return func_([]() { return impl::CaseCounter{}; }).Extract();
  }

  /// Returns a string of comma separated quoted values of Case parameters.
  ///
  /// \b Example: "('a', '1'), ('b', '2'), ('c', '3')"
  ///
  /// Parameters of Case should be formattable.
  std::string Describe() const {
    return func_([]() { return impl::CaseDescriber{}; }).Extract();
  }

  /// Returns a string of comma separated quoted values of first Case
  /// parameters.
  ///
  /// \b Example: "'a', 'b', 'c'"
  ///
  /// First parameters of Case should be formattable.
  std::string DescribeFirst() const {
    return func_([]() { return impl::CaseFirstDescriber{}; }).Extract();
  }

  /// Returns a string of comma separated quoted values of second Case
  /// parameters.
  ///
  /// \b Example: "'1', '2', '3'"
  ///
  /// Second parameters of Case should be formattable.
  std::string DescribeSecond() const {
    return func_([]() { return impl::CaseSecondDescriber{}; }).Extract();
  }

  /// Returns a string of comma separated quoted values of Case
  /// parameters that matches by type.
  ///
  /// \b Example: "'1', '2', '3'"
  ///
  /// Corresponding Case must be formattable
  template <typename T>
  std::string DescribeByType() const {
    if constexpr (std::is_convertible_v<T, First>) {
      return DescribeFirst();
    } else {
      return DescribeSecond();
    }
  }

  constexpr value_type GetValuesByIndex(std::size_t index) const {
    auto result = func_(
        [index]() { return impl::CaseGetValuesByIndex<First, Second>{index}; });
    return value_type{result.GetFirst(), result.GetSecond()};
  }

  class iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

    explicit constexpr iterator(const TrivialBiMap& map, std::size_t position)
        : map_{map}, position_{position} {}

    constexpr bool operator==(iterator other) const {
      return position_ == other.position_;
    }

    constexpr bool operator!=(iterator other) const {
      return position_ != other.position_;
    }

    constexpr iterator operator++() {
      ++position_;
      return *this;
    }

    constexpr iterator operator++(int) {
      iterator copy{*this};
      ++position_;
      return copy;
    }

    constexpr value_type operator*() const {
      return map_.GetValuesByIndex(position_);
    }

   private:
    const TrivialBiMap& map_;
    std::size_t position_;
  };

  constexpr iterator begin() const { return iterator(*this, 0); }
  constexpr iterator end() const { return iterator(*this, size()); }
  constexpr iterator cbegin() const { return begin(); }
  constexpr iterator cend() const { return end(); }

 private:
  const BuilderFunc func_;
};

template <typename BuilderFunc>
TrivialBiMap(BuilderFunc) -> TrivialBiMap<BuilderFunc>;

/// @ingroup userver_universal userver_containers
///
/// @brief Unordered set for trivial types, including string literals.
///
/// For a two-value Case statements or efficiency notes
/// see @ref utils::TrivialBimap.
template <typename BuilderFunc>
class TrivialSet final {
  using TypesPair =
      std::invoke_result_t<const BuilderFunc&, impl::SwitchTypesDetector>;

 public:
  using First = typename TypesPair::first_type;
  using Second = typename TypesPair::second_type;

  constexpr TrivialSet(BuilderFunc&& func) noexcept : func_(std::move(func)) {
    static_assert(std::is_empty_v<BuilderFunc>,
                  "Mapping function should not capture variables");
    static_assert(std::is_trivially_copyable_v<First>,
                  "First type in Case must be trivially copyable");
    static_assert(std::is_void_v<Second>,
                  "Second type in Case should be skipped in utils::TrivialSet");
  }

  constexpr bool Contains(First value) const noexcept {
    return func_(
               [value]() { return impl::SwitchByFirst<First, Second>{value}; })
        .Extract();
  }

  constexpr bool ContainsICase(std::string_view value) const noexcept {
    static_assert(std::is_convertible_v<First, std::string_view>,
                  "ContainsICase works only with std::string_view");

    return func_([value]() { return impl::SwitchByFirstICase<void>{value}; })
        .Extract();
  }

  constexpr std::size_t size() const noexcept {
    return func_([]() { return impl::CaseCounter{}; }).Extract();
  }

  /// Returns a string of comma separated quoted values of Case parameters.
  ///
  /// \b Example: "'a', 'b', 'c'"
  ///
  /// Parameters of Case should be formattable.
  std::string Describe() const {
    return func_([]() { return impl::CaseFirstDescriber{}; }).Extract();
  }

  /// Returns index of the value in Case parameters or std::nullopt if no such
  /// value.
  constexpr std::optional<std::size_t> GetIndex(First value) const {
    return func_([value]() { return impl::CaseFirstIndexer{value}; }).Extract();
  }

 private:
  const BuilderFunc func_;
};

template <typename BuilderFunc>
TrivialSet(BuilderFunc) -> TrivialSet<BuilderFunc>;

/// @brief Parses and returns whatever is specified by `map` from a
/// `formats::*::Value`.
/// @throws ExceptionType or `Value::Exception` by default, if `value` is not a
/// string, or if `value` is not contained in `map`.
/// @see @ref scripts/docs/en/userver/formats.md
template <typename ExceptionType = void, typename Value, typename BuilderFunc>
auto ParseFromValueString(const Value& value, TrivialBiMap<BuilderFunc> map) {
  if constexpr (!std::is_void_v<ExceptionType>) {
    if (!value.IsString()) {
      throw ExceptionType(fmt::format(
          "Invalid value at '{}': expected a string", value.GetPath()));
    }
  }

  const auto string = value.template As<std::string>();
  const auto parsed = map.TryFind(string);
  if (parsed) return *parsed;

  using Exception =
      std::conditional_t<std::is_void_v<ExceptionType>,
                         typename Value::Exception, ExceptionType>;

  throw Exception(fmt::format(
      "Invalid value of {} at '{}': '{}' is not one of {}",
      compiler::GetTypeName<std::decay_t<decltype(*parsed)>>(), value.GetPath(),
      string, map.template DescribeByType<std::string>()));
}

namespace impl {

// @brief Converts `value` to `std::string_view` using `map`. If `value` is not
// contained in `map`, then crashes the service in Debug builds, or throws
// utils::InvariantError in Release builds.
template <typename Enum, typename BuilderFunc>
std::string_view EnumToStringView(Enum value, TrivialBiMap<BuilderFunc> map) {
  static_assert(std::is_enum_v<Enum>);
  if (const auto string = map.TryFind(value)) return *string;

  UINVARIANT(
      false,
      fmt::format("Invalid value of enum {}: {}", compiler::GetTypeName<Enum>(),
                  static_cast<std::underlying_type_t<Enum>>(value)));
}

template <typename Selector, class Keys, typename Values,
          std::size_t... Indices>
constexpr auto TrivialBiMapMultiCase(Selector selector, const Keys& keys,
                                     const Values& values,
                                     std::index_sequence<0, Indices...>) {
  auto selector2 = selector.Case(std::data(keys)[0], std::data(values)[0]);
  ((selector2 =
        selector2.Case(std::data(keys)[Indices], std::data(values)[Indices])),
   ...);
  return selector2;
}

template <const auto& Keys, const auto& Values>
struct TrivialBiMapMultiCaseDispatch {
  template <class Selector>
  constexpr auto operator()(Selector selector) const {
    constexpr auto kKeysSize = std::size(Keys);
    return impl::TrivialBiMapMultiCase(selector(), Keys, Values,
                                       std::make_index_sequence<kKeysSize>{});
  }
};

template <typename Selector, class Values, std::size_t... Indices>
constexpr auto TrivialSetMultiCase(Selector selector, const Values& values,
                                   std::index_sequence<0, Indices...>) {
  auto selector2 = selector.Case(std::data(values)[0]);
  ((selector2 = selector2.Case(std::data(values)[Indices])), ...);
  return selector2;
}

template <const auto& Values>
struct TrivialSetMultiCaseDispatch {
  template <class Selector>
  constexpr auto operator()(Selector selector) const {
    constexpr auto kValuesSize = std::size(Values);
    return impl::TrivialSetMultiCase(selector(), Values,
                                     std::make_index_sequence<kValuesSize>{});
  }
};

}  // namespace impl

/// @brief Zips two global `constexpr` arrays into an utils::TrivialBiMap.
template <const auto& Keys, const auto& Values>
constexpr auto MakeTrivialBiMap() {
  static_assert(std::size(Keys) == std::size(Values));
  static_assert(std::size(Keys) >= 1);
  return TrivialBiMap(impl::TrivialBiMapMultiCaseDispatch<Keys, Values>{});
}

template <const auto& Values>
constexpr auto MakeTrivialSet() {
  return TrivialSet(impl::TrivialSetMultiCaseDispatch<Values>{});
}

}  // namespace utils

USERVER_NAMESPACE_END
