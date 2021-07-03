#pragma once

/// @file utils/any_movable.hpp
/// @brief @copybrief utils::AnyMovable

#include <any>  // for std::bad_any_cast
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

/// Utilities
namespace utils {

/// @ingroup userver_containers
///
/// @brief Replacement for boost::any and std::any that does not require
/// copy-constructor from a held type.
///
/// Usage example:
///   utils::AnyMovable a{std::string("Hello")};
///   UASSERT(utils::AnyMovableCast<std::string>(a) == "Hello");
///   UASSERT(!utils::AnyMovableCast<int>(&a));
class AnyMovable {
 public:
  AnyMovable() = default;
  AnyMovable(AnyMovable&&) = default;
  AnyMovable& operator=(AnyMovable&&) = default;

  template <
      class ValueType,
      class /*Enable*/ = std::enable_if_t<
          !std::is_same<AnyMovable, std::decay_t<ValueType>>::value &&
          !std::is_same<AnyMovable*, std::decay_t<ValueType>>::value &&
          !std::is_same<const AnyMovable*, std::decay_t<ValueType>>::value>>
  /* implicit */ AnyMovable(ValueType&& value)
      : content_(std::make_unique<Holder<std::decay_t<ValueType>>>(
            std::forward<ValueType>(value))) {}

  template <class ValueType>
  AnyMovable& operator=(ValueType&& rhs) {
    *this = AnyMovable(std::forward<ValueType>(rhs));
    return *this;
  }

  bool IsEmpty() const noexcept { return !content_; }

  void Clear() noexcept { *this = AnyMovable(); }

  const std::type_info& Type() const noexcept {
    return content_ ? content_->Type() : typeid(void);
  }

 private:  // types
  class HolderBase {
   public:
    HolderBase() = default;
    HolderBase(const HolderBase&) = delete;
    HolderBase(HolderBase&&) = delete;
    HolderBase& operator=(const HolderBase&) = delete;
    HolderBase& operator=(HolderBase&&) = delete;

    virtual ~HolderBase() = default;

    virtual const std::type_info& Type() const noexcept = 0;
  };

  template <typename ValueType>
  class Holder final : public HolderBase {
   public:
    Holder(const ValueType& value) : held_(value) {}

    Holder(ValueType&& value) : held_(std::move(value)) {}

    const std::type_info& Type() const noexcept override {
      return typeid(ValueType);
    }

    ValueType held_;
  };

  template <typename ValueType>
  friend ValueType* AnyMovableCast(AnyMovable*) noexcept;

  std::unique_ptr<HolderBase> content_;
};

class BadAnyMovableCast : public std::bad_any_cast {
 public:
  const char* what() const noexcept override {
    return "utils::bad_any_movable_cast: "
           "failed conversion using utils::any_movable_cast";
  }
};

/// @return nullptr if operand is nullptr or type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
ValueType* AnyMovableCast(AnyMovable* operand) noexcept {
  using HeldType = AnyMovable::Holder<std::remove_cv_t<ValueType>>;
  return (operand && operand->Type() == typeid(ValueType))
             ? std::addressof(static_cast<HeldType&>(*operand->content_).held_)
             : nullptr;
}

/// @return nullptr if operand is nullptr or type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
const ValueType* AnyMovableCast(const AnyMovable* operand) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return AnyMovableCast<ValueType>(const_cast<AnyMovable*>(operand));
}

/// @throw BadAnyMovableCast if type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
ValueType AnyMovableCast(AnyMovable& operand) {
  using NonRef = std::remove_reference_t<ValueType>;

  auto* result = AnyMovableCast<NonRef>(std::addressof(operand));
  if (!result) throw BadAnyMovableCast();

  if constexpr (std::is_rvalue_reference<ValueType>::value) {
    return std::move(*result);
  } else {
    return *result;
  }
}

/// @throw BadAnyMovableCast if type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
ValueType AnyMovableCast(const AnyMovable& operand) {
  using NonRef = std::remove_reference_t<ValueType>;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return AnyMovableCast<const NonRef&>(const_cast<AnyMovable&>(operand));
}

/// @throw BadAnyMovableCast if type of the data stored in operand
/// does not match ValueType
template <typename ValueType>
ValueType&& AnyMovableCast(AnyMovable&& operand) {
  static_assert(std::is_rvalue_reference<ValueType&&>::value ||
                    std::is_const<std::remove_reference_t<ValueType>>::value,
                "utils::any_movable_cast shall not be used for getting "
                "nonconst references to temporary objects");
  return AnyMovableCast<ValueType&&>(operand);
}

}  // namespace utils
