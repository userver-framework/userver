#pragma once

/// @file userver/utils/any_movable.hpp
/// @brief @copybrief utils::AnyMovable

#include <any>  // for std::bad_any_cast
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

/// Utilities
namespace utils {

/// @ingroup userver_containers
///
/// @brief Replacement for `std::any` that is not copyable. It allows to store
/// non-copyable and even non-movable types.
///
/// Usage example:
/// @snippet shared/src/utils/any_movable_test.cpp  AnyMovable example usage
class AnyMovable final {
 public:
  /// Creates an empty `AnyMovable`
  constexpr AnyMovable() noexcept = default;

  /// `AnyMovable` is movable, but not copyable
  AnyMovable(AnyMovable&&) noexcept = default;
  AnyMovable& operator=(AnyMovable&&) noexcept = default;

  /// Copies or moves the provided object inside the `AnyMovable`. `const`,
  /// reference, arrays and function pointers are decayed.
  template <typename ValueType, typename = std::enable_if_t<!std::is_same_v<
                                    AnyMovable, std::decay_t<ValueType>>>>
  /*implicit*/ AnyMovable(ValueType&& value);

  /// In-place constructs an object of the specified type
  template <typename ValueType, typename... Args>
  explicit AnyMovable(std::in_place_type_t<ValueType> tag, Args&&... args);

  /// In-place constructs an object of the specified type
  template <typename ValueType, typename Item, typename... Args>
  explicit AnyMovable(std::in_place_type_t<ValueType> tag,
                      std::initializer_list<Item> list, Args&&... args);

  /// Copies or moves the provided object inside the `AnyMovable`. `const`,
  /// reference, arrays and function pointers are decayed.
  template <typename ValueType>
  AnyMovable& operator=(ValueType&& rhs);

  /// Check if the `AnyMovable` is empty
  bool HasValue() const noexcept;

  /// Destroy the old contents, making `*this` empty
  void Reset() noexcept;

  /// In-place constructs an object of the specified type
  template <typename ValueType, typename... Args>
  void Emplace(Args&&... args);

  /// In-place constructs an object of the specified type
  template <typename ValueType, typename Item, typename... Args>
  void Emplace(std::initializer_list<Item> list, Args&&... args);

 private:
  struct HolderBase;

  template <typename ValueType>
  struct Holder;

  struct HolderDeleter {
    void operator()(HolderBase* holder) noexcept;
  };

  template <typename ValueType>
  friend ValueType* AnyCast(AnyMovable*) noexcept;

  template <typename ValueType>
  friend const ValueType* AnyCast(const AnyMovable*) noexcept;

  std::unique_ptr<HolderBase, HolderDeleter> content_;
};

/// @brief The exception that is thrown when `AnyCast` fails
class BadAnyMovableCast final : public std::bad_any_cast {
 public:
  const char* what() const noexcept override;
};

/// @return nullptr if operand is nullptr or type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
ValueType* AnyCast(AnyMovable* operand) noexcept;

/// @return nullptr if operand is nullptr or type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
const ValueType* AnyCast(const AnyMovable* operand) noexcept;

/// @note Cast to a reference type to avoid extra copies
/// @throw BadAnyMovableCast if type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
ValueType AnyCast(AnyMovable& operand);

/// @note Cast to a reference type to avoid extra copies
/// @throw BadAnyMovableCast if type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
ValueType AnyCast(const AnyMovable& operand);

/// @note Cast to a reference type to avoid extra moves
/// @throw BadAnyMovableCast if type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
ValueType AnyCast(AnyMovable&& operand);

struct AnyMovable::HolderBase {
  using DeleterType = void (*)(HolderBase&) noexcept;

  DeleterType deleter;
};

template <typename ValueType>
struct AnyMovable::Holder final : public HolderBase {
  static_assert(std::is_same_v<ValueType, std::decay_t<ValueType>>,
                "The requested type can't be stored in an AnyMovable");

  ValueType held;

  static void Deleter(HolderBase& holder) noexcept {
    delete &static_cast<Holder&>(holder);
  }

  template <typename... Args>
  static std::unique_ptr<HolderBase, HolderDeleter> Make(Args&&... args) {
    return std::unique_ptr<HolderBase, HolderDeleter>{
        // intended raw ctor call, sometimes casts
        // NOLINTNEXTLINE(google-readability-casting)
        new Holder{{&Deleter}, ValueType(std::forward<Args>(args)...)}};
  }

  static ValueType* GetIf(const AnyMovable* any) noexcept {
    return (any && any->content_ && any->content_->deleter == &Deleter)
               ? &static_cast<Holder&>(*any->content_).held
               : nullptr;
  }
};

template <typename ValueType, typename>
AnyMovable::AnyMovable(ValueType&& value)
    : content_(Holder<std::decay_t<ValueType>>::Make(
          std::forward<ValueType>(value))) {
  static_assert(
      !std::is_same_v<AnyMovable*, std::decay_t<ValueType>> &&
          !std::is_same_v<const AnyMovable*, std::decay_t<ValueType>>,
      "AnyMovable misuse detected: trying to wrap AnyMovable* in another "
      "AnyMovable. The pointer was probably meant to be dereferenced.");
}

template <typename ValueType, typename... Args>
AnyMovable::AnyMovable(std::in_place_type_t<ValueType> /*tag*/, Args&&... args)
    : content_(Holder<ValueType>::Make(std::forward<Args>(args)...)) {}

template <typename ValueType, typename Item, typename... Args>
AnyMovable::AnyMovable(std::in_place_type_t<ValueType> /*tag*/,
                       std::initializer_list<Item> list, Args&&... args)
    : content_(Holder<ValueType>::Make(list, std::forward<Args>(args)...)) {}

template <typename ValueType>
AnyMovable& AnyMovable::operator=(ValueType&& rhs) {
  *this = AnyMovable(std::forward<ValueType>(rhs));
  return *this;
}

template <typename ValueType, typename... Args>
void AnyMovable::Emplace(Args&&... args) {
  content_ = Holder<ValueType>::Make(std::forward<Args>(args)...);
}

template <typename ValueType, typename Item, typename... Args>
void AnyMovable::Emplace(std::initializer_list<Item> list, Args&&... args) {
  content_ = Holder<ValueType>::Make(list, std::forward<Args>(args)...);
}

template <typename ValueType>
ValueType* AnyCast(AnyMovable* operand) noexcept {
  return AnyMovable::Holder<ValueType>::GetIf(operand);
}

template <typename ValueType>
const ValueType* AnyCast(const AnyMovable* operand) noexcept {
  return AnyMovable::Holder<ValueType>::GetIf(operand);
}

template <typename ValueType>
// might be requested by user
// NOLINTNEXTLINE(readability-const-return-type)
ValueType AnyCast(AnyMovable& operand) {
  using NonRef = std::remove_cv_t<std::remove_reference_t<ValueType>>;
  auto* result = AnyCast<NonRef>(&operand);
  if (!result) throw BadAnyMovableCast();
  return static_cast<ValueType>(*result);
}

template <typename ValueType>
// might be requested by user
// NOLINTNEXTLINE(readability-const-return-type)
ValueType AnyCast(const AnyMovable& operand) {
  using NonRef = std::remove_cv_t<std::remove_reference_t<ValueType>>;
  auto* result = AnyCast<NonRef>(&operand);
  if (!result) throw BadAnyMovableCast();
  return static_cast<ValueType>(*result);
}

template <typename ValueType>
ValueType AnyCast(AnyMovable&& operand) {
  using NonRef = std::remove_cv_t<std::remove_reference_t<ValueType>>;
  auto* result = AnyCast<NonRef>(&operand);
  if (!result) throw BadAnyMovableCast();
  return static_cast<ValueType>(std::move(*result));
}

}  // namespace utils

USERVER_NAMESPACE_END
