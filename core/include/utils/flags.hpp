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
  Flags& Clear(Flags);

  Flags operator|(Flags) const;
  Flags operator&(Flags) const;

  bool operator==(Flags) const;
  bool operator!=(Flags) const;

  ValueType GetValue();

 private:
  friend class AtomicFlags<Enum>;

  ValueType value_;
};

template <typename Enum>
Flags<Enum> operator|(Enum, Flags<Enum>);

template <typename Enum>
Flags<Enum> operator&(Enum, Flags<Enum>);

template <typename Enum>
bool operator==(Enum, Flags<Enum>);

template <typename Enum>
bool operator!=(Enum, Flags<Enum>);

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
  Flags<Enum> Exchange(Flags<Enum>);

  AtomicFlags& operator|=(Flags<Enum>);
  AtomicFlags& operator&=(Flags<Enum>);
  AtomicFlags& Clear(Flags<Enum>);
  Flags<Enum> FetchOr(Flags<Enum>);
  Flags<Enum> FetchAnd(Flags<Enum>);
  Flags<Enum> FetchClear(Flags<Enum>);

  Flags<Enum> operator|(Flags<Enum>) const;
  Flags<Enum> operator&(Flags<Enum>)const;

  bool operator==(Flags<Enum>) const;
  bool operator!=(Flags<Enum>) const;

  ValueType GetValue();

 private:
  std::atomic<ValueType> value_;
};

template <typename Enum>
Flags<Enum> operator|(Enum, const AtomicFlags<Enum>&);

template <typename Enum>
Flags<Enum> operator&(Enum, const AtomicFlags<Enum>&);

template <typename Enum>
bool operator==(Enum, const AtomicFlags<Enum>&);

template <typename Enum>
bool operator!=(Enum, const AtomicFlags<Enum>&);

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
Flags<Enum>& Flags<Enum>::Clear(Flags flags) {
  value_ &= ~flags.value_;
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
bool Flags<Enum>::operator==(Flags rhs) const {
  return value_ == rhs.value_;
}

template <typename Enum>
bool Flags<Enum>::operator!=(Flags rhs) const {
  return !(*this == rhs);
}

template <typename Enum>
typename Flags<Enum>::ValueType Flags<Enum>::GetValue() {
  return this->value_;
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
bool operator==(Enum lhs, Flags<Enum> rhs) {
  return rhs == lhs;
}

template <typename Enum>
bool operator!=(Enum lhs, Flags<Enum> rhs) {
  return rhs != lhs;
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
Flags<Enum> AtomicFlags<Enum>::Exchange(Flags<Enum> flags) {
  return static_cast<Enum>(value_.exchange(flags.value_));
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
AtomicFlags<Enum>& AtomicFlags<Enum>::Clear(Flags<Enum> flags) {
  FetchClear(flags);
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
Flags<Enum> AtomicFlags<Enum>::FetchClear(Flags<Enum> flags) {
  return static_cast<Enum>(value_.fetch_and(~flags.value_));
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
bool AtomicFlags<Enum>::operator==(Flags<Enum> rhs) const {
  return value_ == rhs.value_;
}

template <typename Enum>
bool AtomicFlags<Enum>::operator!=(Flags<Enum> rhs) const {
  return !(*this == rhs);
}

template <typename Enum>
typename AtomicFlags<Enum>::ValueType AtomicFlags<Enum>::GetValue() {
  return this->value_.load();
}

template <typename Enum>
Flags<Enum> operator|(Enum lhs, const AtomicFlags<Enum>& rhs) {
  return rhs | lhs;
}

template <typename Enum>
Flags<Enum> operator&(Enum lhs, const AtomicFlags<Enum>& rhs) {
  return rhs & lhs;
}

template <typename Enum>
bool operator==(Enum lhs, const AtomicFlags<Enum>& rhs) {
  return rhs == lhs;
}

template <typename Enum>
bool operator!=(Enum lhs, const AtomicFlags<Enum>& rhs) {
  return rhs != lhs;
}

}  // namespace utils
