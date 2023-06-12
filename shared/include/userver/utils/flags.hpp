#pragma once

/// @file userver/utils/flags.hpp
/// @brief Types that provide flags interface to enums

#include <atomic>
#include <initializer_list>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

template <typename Enum>
class AtomicFlags;

/// @ingroup userver_containers
///
/// @brief Wrapper to extend enum with flags interface
template <typename Enum>
class Flags final {
 public:
  using ValueType = std::underlying_type_t<Enum>;

  constexpr Flags() noexcept : Flags(Enum::kNone) {}
  /*implicit*/ constexpr Flags(Enum) noexcept;
  constexpr Flags(std::initializer_list<Enum>) noexcept;

  constexpr explicit operator bool() const;

  constexpr Flags& operator|=(Flags);
  constexpr Flags& operator&=(Flags);
  constexpr Flags& Clear(Flags);

  constexpr Flags operator|(Flags) const;
  constexpr Flags operator&(Flags) const;

  constexpr bool operator==(Flags) const;
  constexpr bool operator!=(Flags) const;

  constexpr ValueType GetValue() const;

 private:
  friend class AtomicFlags<Enum>;

  ValueType value_;
};

template <typename Enum>
constexpr Flags<Enum> operator|(Enum, Flags<Enum>);

template <typename Enum>
constexpr Flags<Enum> operator&(Enum, Flags<Enum>);

template <typename Enum>
constexpr bool operator==(Enum, Flags<Enum>);

template <typename Enum>
constexpr bool operator!=(Enum, Flags<Enum>);

/// @ingroup userver_containers
///
/// @brief Wrapper to extend enum with atomic flags interface
template <typename Enum>
class AtomicFlags final {
 public:
  using ValueType = std::underlying_type_t<Enum>;

  constexpr AtomicFlags() : AtomicFlags(Enum::kNone) {}
  constexpr explicit AtomicFlags(Enum);
  constexpr AtomicFlags(std::initializer_list<Enum>);

  explicit operator bool() const;
  /*implicit*/ operator Flags<Enum>() const;
  Flags<Enum> Load(std::memory_order = std::memory_order_seq_cst) const;

  AtomicFlags& operator=(Flags<Enum>);
  AtomicFlags& Store(Flags<Enum>,
                     std::memory_order = std::memory_order_seq_cst);
  Flags<Enum> Exchange(Flags<Enum>);

  AtomicFlags& operator|=(Flags<Enum>);
  AtomicFlags& operator&=(Flags<Enum>);
  AtomicFlags& Clear(Flags<Enum>);
  Flags<Enum> FetchOr(Flags<Enum>,
                      std::memory_order = std::memory_order_seq_cst);
  Flags<Enum> FetchAnd(Flags<Enum>,
                       std::memory_order = std::memory_order_seq_cst);
  Flags<Enum> FetchClear(Flags<Enum>,
                         std::memory_order = std::memory_order_seq_cst);
  bool CompareExchangeWeak(Flags<Enum>& expected, Flags<Enum> desired,
                           std::memory_order order = std::memory_order_seq_cst);
  bool CompareExchangeStrong(
      Flags<Enum>& expected, Flags<Enum> desired,
      std::memory_order order = std::memory_order_seq_cst);
  bool CompareExchangeWeak(Flags<Enum>& expected, Flags<Enum> desired,
                           std::memory_order success,
                           std::memory_order failure);
  bool CompareExchangeStrong(Flags<Enum>& expected, Flags<Enum> desired,
                             std::memory_order success,
                             std::memory_order failure);

  Flags<Enum> operator|(Flags<Enum>) const;
  Flags<Enum> operator&(Flags<Enum>) const;

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
constexpr Flags<Enum>::Flags(Enum value) noexcept
    : value_(static_cast<ValueType>(value)) {}

template <typename Enum>
constexpr Flags<Enum>::Flags(std::initializer_list<Enum> values) noexcept
    : Flags() {
  for (Enum value : values) {
    *this |= value;
  }
}

template <typename Enum>
constexpr Flags<Enum>::operator bool() const {
  return !!value_;
}

template <typename Enum>
constexpr Flags<Enum>& Flags<Enum>::operator|=(Flags rhs) {
  value_ |= rhs.value_;
  return *this;
}

template <typename Enum>
constexpr Flags<Enum>& Flags<Enum>::operator&=(Flags rhs) {
  value_ &= rhs.value_;
  return *this;
}

template <typename Enum>
constexpr Flags<Enum>& Flags<Enum>::Clear(Flags flags) {
  value_ &= ~flags.value_;
  return *this;
}

template <typename Enum>
constexpr Flags<Enum> Flags<Enum>::operator|(Flags rhs) const {
  return Flags(*this) |= rhs;
}

template <typename Enum>
constexpr Flags<Enum> Flags<Enum>::operator&(Flags rhs) const {
  return Flags(*this) &= rhs;
}

template <typename Enum>
constexpr bool Flags<Enum>::operator==(Flags rhs) const {
  return value_ == rhs.value_;
}

template <typename Enum>
constexpr bool Flags<Enum>::operator!=(Flags rhs) const {
  return !(*this == rhs);
}

template <typename Enum>
constexpr typename Flags<Enum>::ValueType Flags<Enum>::GetValue() const {
  return this->value_;
}

template <typename Enum>
constexpr Flags<Enum> operator|(Enum lhs, Flags<Enum> rhs) {
  return rhs |= lhs;
}

template <typename Enum>
constexpr Flags<Enum> operator&(Enum lhs, Flags<Enum> rhs) {
  return rhs &= lhs;
}

template <typename Enum>
constexpr bool operator==(Enum lhs, Flags<Enum> rhs) {
  return rhs == Flags<Enum>{lhs};
}

template <typename Enum>
constexpr bool operator!=(Enum lhs, Flags<Enum> rhs) {
  return rhs != Flags<Enum>{lhs};
}

template <typename Enum>
constexpr AtomicFlags<Enum>::AtomicFlags(Enum value)
    : value_(static_cast<ValueType>(value)) {}

template <typename Enum>
constexpr AtomicFlags<Enum>::AtomicFlags(std::initializer_list<Enum> values)
    : AtomicFlags(Enum(values)) {}

template <typename Enum>
AtomicFlags<Enum>::operator bool() const {
  return !!value_;
}

template <typename Enum>
AtomicFlags<Enum>::operator Flags<Enum>() const {
  return Load();
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::Load(std::memory_order order) const {
  return static_cast<Enum>(value_.load(order));
}

template <typename Enum>
AtomicFlags<Enum>& AtomicFlags<Enum>::operator=(Flags<Enum> rhs) {
  Store(rhs);
  return *this;
}

template <typename Enum>
AtomicFlags<Enum>& AtomicFlags<Enum>::Store(Flags<Enum> rhs,
                                            std::memory_order order) {
  value_.store(rhs.value_, order);
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
Flags<Enum> AtomicFlags<Enum>::FetchOr(Flags<Enum> rhs,
                                       std::memory_order memory_order) {
  return static_cast<Enum>(value_.fetch_or(rhs.value_, memory_order));
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::FetchAnd(Flags<Enum> rhs,
                                        std::memory_order memory_order) {
  return static_cast<Enum>(value_.fetch_and(rhs.value_, memory_order));
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::FetchClear(Flags<Enum> flags,
                                          std::memory_order memory_order) {
  return static_cast<Enum>(value_.fetch_and(~flags.value_, memory_order));
}

template <typename Enum>
bool AtomicFlags<Enum>::CompareExchangeWeak(Flags<Enum>& expected,
                                            Flags<Enum> desired,
                                            std::memory_order order) {
  auto expected_int = expected.GetValue();
  const bool result =
      value_.compare_exchange_weak(expected_int, desired.GetValue(), order);
  expected = Enum{expected_int};
  return result;
}

template <typename Enum>
bool AtomicFlags<Enum>::CompareExchangeStrong(Flags<Enum>& expected,
                                              Flags<Enum> desired,
                                              std::memory_order order) {
  auto expected_int = expected.GetValue();
  const bool result =
      value_.compare_exchange_strong(expected_int, desired.GetValue(), order);
  expected = Enum{expected_int};
  return result;
}

template <typename Enum>
bool AtomicFlags<Enum>::CompareExchangeWeak(Flags<Enum>& expected,
                                            Flags<Enum> desired,
                                            std::memory_order success,
                                            std::memory_order failure) {
  auto expected_int = expected.GetValue();
  const bool result = value_.compare_exchange_weak(
      expected_int, desired.GetValue(), success, failure);
  expected = Enum{expected_int};
  return result;
}

template <typename Enum>
bool AtomicFlags<Enum>::CompareExchangeStrong(Flags<Enum>& expected,
                                              Flags<Enum> desired,
                                              std::memory_order success,
                                              std::memory_order failure) {
  auto expected_int = expected.GetValue();
  const bool result = value_.compare_exchange_strong(
      expected_int, desired.GetValue(), success, failure);
  expected = Enum{expected_int};
  return result;
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::operator|(Flags<Enum> rhs) const {
  return Flags<Enum>{*this} |= rhs;
}

template <typename Enum>
Flags<Enum> AtomicFlags<Enum>::operator&(Flags<Enum> rhs) const {
  return Flags<Enum>{*this} &= rhs;
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
  return rhs == Flags<Enum>{lhs};
}

template <typename Enum>
bool operator!=(Enum lhs, const AtomicFlags<Enum>& rhs) {
  return rhs != Flags<Enum>{lhs};
}

}  // namespace utils

USERVER_NAMESPACE_END
