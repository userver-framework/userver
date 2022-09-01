#pragma once

#include <atomic>
#include <type_traits>

#if defined(USERVER_USE_BOOST_DWCAS)
#include <boost/atomic/atomic.hpp>
#endif

#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

#if defined(USERVER_USE_BOOST_DWCAS)
template <typename T>
using InternalDoubleWidthAtomic = boost::atomic<T>;

inline boost::memory_order ToInternalMemoryOrder(
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
  utils::impl::AbortWithStacktrace("Invalid memory order");
}
#else
template <typename T>
using InternalDoubleWidthAtomic = std::atomic<T>;

inline std::memory_order ToInternalMemoryOrder(
    std::memory_order order) noexcept {
  return order;
}
#endif

// Used to get various compilers to produce DWCAS instructions.
// See also RequireDWCAS.cmake.
//
// Has a std::atomic-compatible interface.
template <typename T>
class DoubleWidthAtomic final {
  static_assert(sizeof(T) == sizeof(void*) * 2);
  static_assert(alignof(T) == sizeof(T));
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::has_unique_object_representations_v<T>);

 public:
  constexpr DoubleWidthAtomic(T desired) noexcept : impl_(desired) {}

  DoubleWidthAtomic(const DoubleWidthAtomic&) = delete;

  bool compare_exchange_strong(T& expected, T desired,
                               std::memory_order success,
                               std::memory_order failure) noexcept {
    return impl_.compare_exchange_strong(expected, desired,
                                         ToInternalMemoryOrder(success),
                                         ToInternalMemoryOrder(failure));
  }

  T load(std::memory_order order) const noexcept {
    return impl_.load(ToInternalMemoryOrder(order));
  }

 private:
  mutable InternalDoubleWidthAtomic<T> impl_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
