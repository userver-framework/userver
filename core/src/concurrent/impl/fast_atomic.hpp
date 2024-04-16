#pragma once

#include <atomic>
#include <type_traits>

#ifndef USERVER_USE_STD_DWCAS
#include <boost/atomic/atomic.hpp>
#include <boost/version.hpp>
#endif

#include <userver/compiler/impl/tsan.hpp>
#include <utils/impl/assert_extra.hpp>

#ifdef USERVER_PROTECT_DWCAS
#define USERVER_IMPL_PROTECT_DWCAS_ATTR __attribute__((noinline, flatten))
#else
#define USERVER_IMPL_PROTECT_DWCAS_ATTR __attribute__((always_inline, flatten))
#endif

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

#ifdef USERVER_USE_STD_DWCAS

constexpr std::memory_order ToInternalMemoryOrder(
    std::memory_order order) noexcept {
  return order;
}

template <typename T>
using DoubleWidthCapableAtomic = std::atomic<T>;

template <typename T>
using DoubleWidthCapableAtomicRef = std::atomic<T>&;

template <typename T>
T LoadWithTearing(const DoubleWidthCapableAtomic<T>& storage) {
  // NOTE: this uses the heavy locking CAS instruction.
  return storage.load(std::memory_order_relaxed);
}

#else  // USERVER_USE_STD_DWCAS

constexpr boost::memory_order ToInternalMemoryOrder(
    std::memory_order order) noexcept {
  switch (order) {
    case std::memory_order_relaxed:
      return boost::memory_order_relaxed;
    case std::memory_order_consume:
      return boost::memory_order_consume;
    case std::memory_order_acquire:
      return boost::memory_order_acquire;
    case std::memory_order_release:
      return boost::memory_order_release;
    case std::memory_order_acq_rel:
      return boost::memory_order_acq_rel;
    case std::memory_order_seq_cst:
      return boost::memory_order_seq_cst;
  }
  // AbortWithStacktrace here leads to a compilation error on GCC 8.
  return boost::memory_order_seq_cst;
}

template <typename T>
using DoubleWidthCapableAtomic = boost::atomic<T>;

template <typename T>
using DoubleWidthCapableAtomicRef = boost::atomic<T>&;

#if BOOST_VERSION >= 107400
template <typename T>
T LoadWithTearing(const DoubleWidthCapableAtomic<T>& storage) {
  return storage.value();
}
#else   // BOOST_VERSION >= 107400
template <typename T>
T LoadWithTearing(const DoubleWidthCapableAtomic<T>& storage) {
  // NOTE: this uses the heavy locking CAS instruction.
  return storage.load(boost::memory_order_relaxed);
}
#endif  // BOOST_VERSION >= 107400

#endif  // USERVER_USE_STD_DWCAS

// Tries harder than std::atomic to be lock-free.
// See also RequireDWCAS.cmake.
template <typename T>
class FastAtomic final {
  static_assert(sizeof(T) <= sizeof(void*) * 2);
  // NOLINTNEXTLINE(misc-redundant-expression)
  static_assert(alignof(T) == sizeof(T));
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::has_unique_object_representations_v<T>);

 public:
  constexpr /*implicit*/ FastAtomic(T desired) noexcept : impl_(desired) {}

  FastAtomic(const FastAtomic&) = delete;

  template <std::memory_order Success, std::memory_order Failure>
  USERVER_IMPL_PROTECT_DWCAS_ATTR bool compare_exchange_strong(
      T& expected, T desired) noexcept {
    return static_cast<AtomicRef>(impl_).compare_exchange_strong(
        expected, desired, impl::ToInternalMemoryOrder(Success),
        impl::ToInternalMemoryOrder(Failure));
  }

  template <std::memory_order Order>
  USERVER_IMPL_PROTECT_DWCAS_ATTR T load() const noexcept {
    return static_cast<AtomicRef>(impl_).load(
        impl::ToInternalMemoryOrder(Order));
  }

  template <std::memory_order Order>
  USERVER_IMPL_PROTECT_DWCAS_ATTR T exchange(T desired) noexcept {
    return static_cast<AtomicRef>(impl_).exchange(
        desired, impl::ToInternalMemoryOrder(Order));
  }

  // On supported platforms, tearing can only happen between the data members
  // of a struct value, which can be loaded separately.
  // Memory ordering is equivalent to std::memory_order_relaxed, use
  // std::atomic_thread_fence to gain stricter orderings.
  //
  // Discussion of the legality of such access from the POV of platform ABI:
  // - https://github.com/rust-lang/unsafe-code-guidelines/issues/345
  // - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80835
  // Summary: legal on x86* and ARM.
  USERVER_IMPL_PROTECT_DWCAS_ATTR USERVER_IMPL_DISABLE_TSAN T
  LoadWithTearing() const noexcept {
    return impl::LoadWithTearing(impl_);
  }

 private:
  using AtomicRef = DoubleWidthCapableAtomicRef<T>;

  mutable DoubleWidthCapableAtomic<T> impl_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
