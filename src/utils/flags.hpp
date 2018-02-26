#pragma once

#include <initializer_list>
#include <type_traits>

namespace utils {

template <typename Enum>
class Flags {
 public:
  using ValueType = std::underlying_type_t<Enum>;

  Flags() : Flags(Enum::kDefault) {}
  /*implicit*/ Flags(Enum);
  Flags(std::initializer_list<Enum>);

  explicit operator bool() const;

  Flags& operator|=(Flags);
  Flags& operator&=(Flags);

  Flags operator|(Flags) const;
  Flags operator&(Flags) const;

 private:
  ValueType value_;
};

template <typename Enum>
Flags<Enum> operator|(Enum, Flags<Enum>);

template <typename Enum>
Flags<Enum> operator&(Enum, Flags<Enum>);

template <typename Enum>
Flags<Enum>::Flags(Enum value) : value_(static_cast<ValueType>(value)) {}

template <typename Enum>
Flags<Enum>::Flags(std::initializer_list<Enum> values) : value_(ValueType()) {
  for (Enum value : values) {
    *this |= value;
  }
}

template <typename Enum>
Flags<Enum>::operator bool() const {
  return value_;
}

template <typename Enum>
Flags<Enum>& Flags<Enum>::operator|=(Flags rhs) {
  value_ |= rhs.value_;
  return *this;
}

template <typename Enum>
Flags<Enum>& Flags<Enum>::operator&=(Flags rhs) {
  value_ &= rhs.value_;
  return *this;
}

template <typename Enum>
Flags<Enum> Flags<Enum>::operator|(Flags rhs) const {
  return Flags(*this) |= rhs;
}

template <typename Enum>
Flags<Enum> Flags<Enum>::operator&(Flags rhs) const {
  return Flags(*this) &= rhs;
}

template <typename Enum>
Flags<Enum> operator|(Enum lhs, Flags<Enum> rhs) {
  return Flags<Enum>(lhs) |= rhs;
}

template <typename Enum>
Flags<Enum> operator&(Enum lhs, Flags<Enum> rhs) {
  return Flags<Enum>(lhs) &= rhs;
}

}  // namespace utils
