#pragma once

#include <atomic>
#include <initializer_list>
#include <type_traits>

namespace utils {

template <typename Enum>
class AtomicFlags;

template <typename Enum>
class Flags {
 public:
  using ValueType = std::underlying_type_t<Enum>;

  Flags() : Flags(Enum::kNone) {}
  /*implicit*/ Flags(Enum);
  Flags(std::initializer_list<Enum>);

  explicit operator bool() const;

  Flags& operator|=(Flags);
  Flags& operator&=(Flags);

  Flags operator|(Flags) const;
  Flags operator&(Flags) const;

 private:
  friend class AtomicFlags<Enum>;

  ValueType value_;
};

template <typename Enum>
Flags<Enum> operator|(Enum, Flags<Enum>);

template <typename Enum>
Flags<Enum> operator&(Enum, Flags<Enum>);

template <typename Enum>
class AtomicFlags {
 public:
  using ValueType = std::underlying_type_t<Enum>;

  AtomicFlags() : AtomicFlags(Enum::kNone) {}
  explicit AtomicFlags(Enum);
  AtomicFlags(std::initializer_list<Enum>);

  explicit operator bool() const;
  /*implicit*/ operator Flags<Enum>() const;

  AtomicFlags& operator=(Flags<Enum>);

  AtomicFlags& operator|=(Flags<Enum>);
  AtomicFlags& operator&=(Flags<Enum>);
  Flags<Enum> FetchOr(Flags<Enum>);
  Flags<Enum> FetchAnd(Flags<Enum>);

  Flags<Enum> operator|(Flags<Enum>) const;
  Flags<Enum> operator&(Flags<Enum>)const;

 private:
  std::atomic<ValueType> value_;
};

template <typename Enum>
Flags<Enum> operator|(Enum, const AtomicFlags<Enum>&);

template <typename Enum>
Flags<Enum> operator&(Enum, const AtomicFlags<Enum>&);

template <typename Enum>
Flags<Enum>::Flags(Enum value) : value_(static_cast<ValueType>(value)) {}

template <typename Enum>
Flags<Enum>::Flags(std::initializer_list<Enum> values) : Flags() {
  for (Enum value : values) {
    *this |= value;
  }
}

template <typename Enum>
Flags<Enum>::operator bool() const {
  return !!value_;
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
  return rhs |= lhs;
}

template <typename Enum>
Flags<Enum> operator&(Enum lhs, Flags<Enum> rhs) {
  return rhs &= lhs;
}

template <typename Enum>
AtomicFlags<Enum>::AtomicFlags(Enum value)
    : value_(static_cast<ValueType>(value)) {}

template <typename Enum>
AtomicFlags<Enum>::AtomicFlags(std::initializer_list<Enum> values)
    : AtomicFlags() {
  for (Enum value : values) {
    *this |= value;
  }
}

template <typename Enum>
AtomicFlags<Enum>::operator bool() const {
  return !!value_;
}

template <typename Enum>
AtomicFlags<Enum>::operator Flags<Enum>() const {
  return static_cast<Enum>(value_.load());
}

template <typename Enum>
AtomicFlags<Enum>& AtomicFlags<Enum>::operator=(Flags<Enum> rhs) {
  value_.store(rhs.value_);
  return *this;
}

template <typename Enum>
AtomicFlags<Enum>& AtomicFlags<Enum>::operator|=(Flags<Enum> rhs) {
  FetchOr(rhs);
  return *this;
}

template <typename Enum>
AtomicFlags<Enum>& AtomicFlags<Enum>::operator&=(Flags<Enum> rhs) {
  FetchAnd(rhs);
  return *this;
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::FetchOr(Flags<Enum> rhs) {
  return static_cast<Enum>(value_.fetch_or(rhs.value_));
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::FetchAnd(Flags<Enum> rhs) {
  return static_cast<Enum>(value_.fetch_and(rhs.value_));
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::operator|(Flags<Enum> rhs) const {
  return Flags<Enum>(*this) |= rhs;
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::operator&(Flags<Enum> rhs) const {
  return Flags<Enum>(*this) &= rhs;
}

template <typename Enum>
Flags<Enum> operator|(Enum lhs, const AtomicFlags<Enum>& rhs) {
  return rhs | lhs;
}

template <typename Enum>
Flags<Enum> operator&(Enum lhs, const AtomicFlags<Enum>& rhs) {
  return rhs & lhs;
}

}  // namespace utils
