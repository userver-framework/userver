#pragma once

/// @file utils/strong_typedef.hpp
/// @brief @copybrief utils::StrongTypedef

#include <functional>
#include <iosfwd>
#include <type_traits>
#include <utility>

#include <utils/underlying_value.hpp>
#include <utils/void_t.hpp>

#include <formats/common/meta.hpp>

namespace logging {
class LogHelper;  // Forward declaration
}

namespace utils {

enum class StrongTypedefOps {
  kNoCompare = 0,  /// Forbid all comparisons for StrongTypedef

  kCompareStrong = 1,           /// Allow comparing two StrongTypedef<Tag, T>
  kCompareTransparentOnly = 2,  /// Allow comparing StrongTypedef<Tag, T> and T
  kCompareTransparent = 3,      /// Allow both of the above
};

constexpr bool operator&(StrongTypedefOps op, StrongTypedefOps mask) noexcept {
  return utils::UnderlyingValue(op) & utils::UnderlyingValue(mask);
}

constexpr auto operator|(StrongTypedefOps op1, StrongTypedefOps op2) noexcept {
  return StrongTypedefOps{utils::UnderlyingValue(op1) |
                          utils::UnderlyingValue(op2)};
}

/// @brief Strong typedef for a type T.
///
/// Typical usage:
///   using MyString = utils::StrongTypedef<class MyStringTag, std::string>;
/// Or:
///   struct MyString final : utils::StrongTypedef<MyString, std::string> {
///     using StrongTypedef::StrongTypedef;
///   };
///
/// Has all the:
/// * comparison (see "Operators" below)
/// * hashing
/// * streaming operators
/// * optimizaed logging for LOG_XXX()
///
/// If used with container-like type also has common STL functions:
/// * begin()
/// * end()
/// * cbegin()
/// * cend()
/// * size()
/// * empty()
/// * clear()
/// * operator[]
///
/// Operators:
///   You can customize the operators that are avaialable by passing the third
///   argument of type StrongTypedefOps. See its docs for more info.
template <class Tag, class T,
          StrongTypedefOps Ops = StrongTypedefOps::kCompareStrong,
          class /*Enable*/ = void>
class StrongTypedef;  // Forward declaration

// Helpers
namespace impl::strong_typedef {

template <class T, class /*Enable*/ = void_t<>>
struct InitializerListImpl {
  struct DoNotMatch;
  using type = DoNotMatch;
};

template <class T>
struct InitializerListImpl<T, void_t<typename T::value_type>> {
  using type = std::initializer_list<typename T::value_type>;
};

// We have to invent this alias to avoid hard error in StrongTypedef<Tag, T> for
// types T that do not have T::value_type.
template <class T>
using InitializerList = typename InitializerListImpl<T>::type;

template <class Tag, class T, StrongTypedefOps Ops, class Other>
using EnableTransparentCompare = std::enable_if_t<
    (Ops & StrongTypedefOps::kCompareTransparentOnly) &&
        !std::is_base_of<StrongTypedef<Tag, T, Ops>, Other>::value,
    bool>;

}  // namespace impl::strong_typedef

// Generic implementation for classes
template <class Tag, class T, StrongTypedefOps Ops, class /*Enable*/>
class StrongTypedef {
  static_assert(!std::is_reference<T>::value);
  static_assert(!std::is_pointer<T>::value);

  static_assert(!std::is_reference<Tag>::value);
  static_assert(!std::is_pointer<Tag>::value);

 public:
  using UnderlyingType = T;
  using TagType = Tag;

  StrongTypedef() = default;
  StrongTypedef(const StrongTypedef&) = default;
  StrongTypedef(StrongTypedef&&) = default;
  StrongTypedef& operator=(const StrongTypedef&) = default;
  StrongTypedef& operator=(StrongTypedef&&) = default;

  constexpr StrongTypedef(impl::strong_typedef::InitializerList<T> lst)
      : data_(lst) {}

  template <
      class... Args,
      std::enable_if_t<std::is_constructible<T, Args...>::value, bool> = true>
  explicit constexpr StrongTypedef(Args&&... args) noexcept(
      noexcept(T(std::forward<Args>(args)...)))
      : data_(std::forward<Args>(args)...) {}

  explicit constexpr operator const T&() const& noexcept { return data_; }
  explicit constexpr operator T() && noexcept { return std::move(data_); }
  explicit constexpr operator T&() & noexcept { return data_; }

  constexpr const T& GetUnderlying() const& noexcept { return data_; }
  constexpr T GetUnderlying() && noexcept { return std::move(data_); }
  constexpr T& GetUnderlying() & noexcept { return data_; }

  auto begin() { return data_.begin(); }
  auto end() { return data_.end(); }
  auto begin() const { return data_.begin(); }
  auto end() const { return data_.end(); }
  auto cbegin() const { return data_.cbegin(); }
  auto cend() const { return data_.cend(); }

  auto size() const { return data_.size(); }
  auto empty() const { return data_.empty(); }

  auto clear() { return data_.clear(); }

  template <class Arg>
  decltype(auto) operator[](Arg&& i) {
    return data_[std::forward<Arg>(i)];
  }
  template <class Arg>
  decltype(auto) operator[](Arg&& i) const {
    return data_[std::forward<Arg>(i)];
  }

 private:
  T data_{};
};

/// @cond
// Strong typedef for arithmetic types. Slightly better than
// `enum class C: type{}` because has optimized logging and transparent
// comparison operators by default.
template <class Tag, class T, StrongTypedefOps Ops>
class StrongTypedef<Tag, T, Ops,
                    std::enable_if_t<std::is_arithmetic<T>::value>> {
  static_assert(!std::is_reference<Tag>::value);
  static_assert(!std::is_pointer<Tag>::value);

 public:
  using UnderlyingType = T;
  using TagType = Tag;

  StrongTypedef() = default;
  StrongTypedef(const StrongTypedef&) = default;
  StrongTypedef(StrongTypedef&&) = default;
  StrongTypedef& operator=(const StrongTypedef&) = default;
  StrongTypedef& operator=(StrongTypedef&&) = default;

  explicit constexpr StrongTypedef(T arg) noexcept : data_(arg) {}

  explicit constexpr operator const T&() const noexcept { return data_; }
  explicit constexpr operator T&() noexcept { return data_; }
  constexpr const T& GetUnderlying() const noexcept { return data_; }
  constexpr T& GetUnderlying() noexcept { return data_; }

 private:
  T data_{};
};

/// @endcond

// Relational operators

#define UTILS_STRONG_TYPEDEF_REL_OP(OPERATOR)                                  \
  template <class Tag1, class T1, StrongTypedefOps Ops1, class Tag2, class T2, \
            StrongTypedefOps Ops2>                                             \
  constexpr bool operator OPERATOR(const StrongTypedef<Tag1, T1, Ops1>&,       \
                                   const StrongTypedef<Tag2, T2, Ops2>&) {     \
    static_assert(!sizeof(T1), "Comparing those StrongTypedefs is forbidden"); \
    return false;                                                              \
  }                                                                            \
                                                                               \
  template <class Tag, class T, StrongTypedefOps Ops>                          \
  constexpr std::enable_if_t<Ops & StrongTypedefOps::kCompareStrong, bool>     \
  operator OPERATOR(const StrongTypedef<Tag, T, Ops>& lhs,                     \
                    const StrongTypedef<Tag, T, Ops>& rhs) {                   \
    return lhs.GetUnderlying() OPERATOR rhs.GetUnderlying();                   \
  }                                                                            \
                                                                               \
  template <class Tag, class T, StrongTypedefOps Ops, class Other>             \
  constexpr impl::strong_typedef::EnableTransparentCompare<Tag, T, Ops, Other> \
  operator OPERATOR(const StrongTypedef<Tag, T, Ops>& lhs, const Other& rhs) { \
    return lhs.GetUnderlying() OPERATOR rhs;                                   \
  }                                                                            \
                                                                               \
  template <class Tag, class T, StrongTypedefOps Ops, class Other>             \
  constexpr impl::strong_typedef::EnableTransparentCompare<Tag, T, Ops, Other> \
  operator OPERATOR(const Other& lhs, const StrongTypedef<Tag, T, Ops>& rhs) { \
    return lhs OPERATOR rhs.GetUnderlying();                                   \
  }

UTILS_STRONG_TYPEDEF_REL_OP(==)
UTILS_STRONG_TYPEDEF_REL_OP(!=)
UTILS_STRONG_TYPEDEF_REL_OP(<)
UTILS_STRONG_TYPEDEF_REL_OP(>)
UTILS_STRONG_TYPEDEF_REL_OP(<=)
UTILS_STRONG_TYPEDEF_REL_OP(>=)

#undef UTILS_STRONG_TYPEDEF_REL_OP

/// Ostreams and Logging

template <class Tag, class T, StrongTypedefOps Ops>
std::ostream& operator<<(std::ostream& os,
                         const StrongTypedef<Tag, T, Ops>& v) {
  return os << v.GetUnderlying();
}

template <class Tag, class T, StrongTypedefOps Ops>
logging::LogHelper& operator<<(logging::LogHelper& os,
                               const StrongTypedef<Tag, T, Ops>& v) {
  return os << v.GetUnderlying();
}

// UnderlyingValue
template <class Tag, class T, StrongTypedefOps Ops>
constexpr decltype(auto) UnderlyingValue(
    const StrongTypedef<Tag, T, Ops>& v) noexcept {
  return v.GetUnderlying();
}

template <class Tag, class T, StrongTypedefOps Ops>
constexpr T UnderlyingValue(StrongTypedef<Tag, T, Ops>&& v) noexcept {
  return std::move(v).GetUnderlying();
}

// Serialization

template <typename Tag, typename T, StrongTypedefOps Ops, typename Enable,
          typename ValueType>
std::enable_if_t<::formats::common::kIsFormatValue<ValueType>,
                 StrongTypedef<Tag, T, Ops, Enable>>
Parse(const ValueType& source,
      formats::parse::To<StrongTypedef<Tag, T, Ops, Enable>>) {
  using StrongTypedefType = StrongTypedef<Tag, T, Ops, Enable>;
  return StrongTypedefType{source.template As<T>()};
}

template <typename Tag, typename T, StrongTypedefOps Ops, typename Enable,
          typename TargetType>
TargetType Serialize(const StrongTypedef<Tag, T, Ops, Enable>& object,
                     formats::serialize::To<TargetType>) {
  return typename TargetType::Builder(object.GetUnderlying()).ExtractValue();
}

template <class Tag, class T, StrongTypedefOps Ops>
std::size_t hash_value(const StrongTypedef<Tag, T, Ops>& v) {
  return hash_value(v.GetUnderlying());
}

}  // namespace utils

// Hashing (STD)
namespace std {

template <class Tag, class T, utils::StrongTypedefOps Ops>
struct hash<utils::StrongTypedef<Tag, T, Ops>> : hash<T> {
  std::size_t operator()(const utils::StrongTypedef<Tag, T, Ops>& v) const
      noexcept(
          noexcept(std::declval<const hash<T>>()(std::declval<const T&>()))) {
    return hash<T>::operator()(v.GetUnderlying());
  }
};

}  // namespace std
