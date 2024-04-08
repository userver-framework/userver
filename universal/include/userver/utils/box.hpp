#pragma once

/// @file userver/utils/box.hpp
/// @brief @copybrief utils::Box

#include <memory>
#include <type_traits>
#include <utility>

#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/logging/log_helper_fwd.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename Self, typename... Args>
inline constexpr bool kArgsAreNotSelf =
    ((sizeof...(Args) > 1) || ... || !std::is_same_v<std::decay_t<Args>, Self>);

template <bool Condition, template <typename...> typename Trait,
          typename... Args>
constexpr bool ConjunctionWithTrait() noexcept {
  if constexpr (Condition) {
    return Trait<Args...>::value;
  } else {
    return false;
  }
}

}  // namespace impl

/// @brief Remote storage for a single item. Implemented as a unique pointer
/// that is never `null`, except when moved from.
///
/// Has the semantics of non-optional `T`.
/// Copies the content on copy, compares by the contained value.
///
/// Use in the following cases:
/// - to create recursive types while maintaining value semantics;
/// - to hide the implementation of a class in cpp;
/// - to prevent the large size or alignment of a field from inflating the size
///   or alignment of an object.
///
/// Use utils::UniqueRef instead:
/// - to add a non-movable field to a movable object;
/// - to own an object of a polymorphic base class.
///
/// Usage example:
/// @snippet utils/box_test.cpp  sample
template <typename T>
class Box {
 public:
  /// Allocate a default-constructed value.
  // Would like to use SFINAE here, but std::optional<Box> requests tests for
  // default construction eagerly, which errors out for a forward-declared T.
  Box() : data_(std::make_unique<T>()) {}

  /// Allocate a `T`, copying or moving @a arg.
  template <typename U = T,
            std::enable_if_t<impl::ConjunctionWithTrait<
                                 // Protection against hiding special
                                 // constructors.
                                 impl::kArgsAreNotSelf<Box, U>,
                                 // Only allow the implicit conversion to Box<T>
                                 // if U is implicitly convertible to T. Also,
                                 // support SFINAE.
                                 std::is_convertible, U&&, T>(),
                             int> = 0>
  /*implicit*/ Box(U&& arg)
      : data_(std::make_unique<T>(std::forward<U>(arg))) {}

  /// Allocate the value, emplacing it with the given @a args.
  template <typename... Args,
            std::enable_if_t<impl::ConjunctionWithTrait<
                                 // Protection against hiding special
                                 // constructors.
                                 impl::kArgsAreNotSelf<Box, Args...>,
                                 // Support SFINAE.
                                 std::is_constructible, T, Args&&...>(),
                             int> = 0>
  explicit Box(Args&&... args)
      : data_(std::make_unique<T>(std::forward<Args>(args)...)) {}

  /// Allocate the value as constructed by the given @a factory.
  /// Allows to save an extra move of the contained value.
  template <typename Factory>
  static Box MakeWithFactory(Factory&& factory) {
    return Box(EmplaceFactory{}, std::forward<Factory>(factory));
  }

  Box(Box&& other) noexcept = default;
  Box& operator=(Box&& other) noexcept = default;

  Box(const Box& other) : data_(std::make_unique<T>(*other)) {}

  Box& operator=(const Box& other) {
    *this = Box{other};
    return *this;
  }

  /// Assigns-through to the contained value.
  template <typename U = T,
            std::enable_if_t<impl::ConjunctionWithTrait<  //
                                 impl::ConjunctionWithTrait<
                                     // Protection against hiding
                                     // special constructors.
                                     impl::kArgsAreNotSelf<Box, U>,
                                     // Support SFINAE.
                                     std::is_constructible, T, U>(),
                                 std::is_assignable, T&, U>(),
                             int> = 0>
  Box& operator=(U&& other) {
    if (data_) {
      *data_ = std::forward<U>(other);
    } else {
      data_ = std::make_unique<T>(std::forward<U>(other));
    }
    return *this;
  }

  // Box is always engaged, unless moved-from. Just call *box.
  /*implicit*/ operator bool() const = delete;

  T* operator->() noexcept { return Get(); }
  const T* operator->() const noexcept { return Get(); }

  T& operator*() noexcept { return *Get(); }
  const T& operator*() const noexcept { return *Get(); }

  bool operator==(const Box& other) const { return **this == *other; }

  bool operator!=(const Box& other) const { return **this != *other; }

  bool operator<(const Box& other) const { return **this < *other; }

  bool operator>(const Box& other) const { return **this > *other; }

  bool operator<=(const Box& other) const { return **this <= *other; }

  bool operator>=(const Box& other) const { return **this >= *other; }

 private:
  struct EmplaceFactory final {};

  template <typename Factory>
  explicit Box(EmplaceFactory, Factory&& factory)
      : data_(new T(std::forward<Factory>(factory)())) {}

  T* Get() noexcept {
    UASSERT_MSG(data_, "Accessing a moved-from Box");
    return data_.get();
  }

  const T* Get() const noexcept {
    UASSERT_MSG(data_, "Accessing a moved-from Box");
    return data_.get();
  }

  std::unique_ptr<T> data_;
};

template <typename Value, typename T>
Box<T> Parse(const Value& value, formats::parse::To<Box<T>>) {
  return Box<T>::MakeWithFactory([&value] { return value.template As<T>(); });
}

template <typename Value, typename T>
Value Serialize(const Box<T>& value, formats::serialize::To<Value>) {
  return Serialize(*value, formats::serialize::To<Value>{});
}

template <typename StringBuilder, typename T>
void WriteToStream(const Box<T>& value, StringBuilder& sw) {
  WriteToStream(*value, sw);
}

template <typename T>
logging::LogHelper& operator<<(logging::LogHelper& lh, const Box<T>& box) {
  lh << *box;
  return lh;
}

}  // namespace utils

USERVER_NAMESPACE_END
