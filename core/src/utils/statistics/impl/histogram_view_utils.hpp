#pragma once

#include <algorithm>
#include <cmath>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm/upper_bound.hpp>
#include <boost/range/combine.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/span.hpp>
#include <userver/utils/statistics/histogram_view.hpp>
#include <userver/utils/statistics/impl/histogram_bucket.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl::histogram {

struct Access final {
  static HistogramView MakeView(const Bucket* buckets) noexcept {
    return HistogramView{buckets};
  }

  template <typename AnyHistogramView>
  static auto Buckets(AnyHistogramView view) noexcept {
    const auto size = HistogramView{view}.GetBucketCount();
    return utils::span(view.buckets_ + 1, view.buckets_ + 1 + size);
  }

  template <typename AnyHistogramView>
  static auto Bounds(AnyHistogramView view) noexcept {
    const auto bound_ref_getter = [](auto&& bucket) -> auto& {
      return bucket.upper_bound.bound;
    };
    return Buckets(view) | boost::adaptors::transformed(bound_ref_getter);
  }

  template <typename AnyHistogramView>
  static auto Values(AnyHistogramView view) noexcept {
    return Buckets(view) | boost::adaptors::transformed([&](auto&& bucket) {
             return bucket.counter.load(std::memory_order_relaxed);
           });
  }
};

inline void AddNonAtomic(std::atomic<std::uint64_t>& to, std::uint64_t x) {
  to.store(to.load(std::memory_order_relaxed) + x, std::memory_order_relaxed);
}

inline bool IsBoundPositive(double x) noexcept {
  return std::isnormal(x) && x > 0;
}

inline bool AreBoundsClose(double x, double y) noexcept {
  constexpr double kEps = 1e-6;
  UASSERT(x > 0);
  UASSERT(y > 0);
  return x > y - kEps * y && x < y + kEps * y;
}

inline bool HasSameBounds(HistogramView lhs, HistogramView rhs) {
  return lhs.GetBucketCount() == rhs.GetBucketCount() &&
         boost::range::equal(Access::Bounds(lhs), Access::Bounds(rhs),
                             AreBoundsClose);
}

inline bool HasSameBoundsAndValues(HistogramView lhs, HistogramView rhs) {
  return HasSameBounds(lhs, rhs) &&
         lhs.GetValueAtInf() == rhs.GetValueAtInf() &&
         boost::range::equal(Access::Values(lhs), Access::Values(rhs));
}

class MutableView final {
 public:
  explicit MutableView(Bucket* buckets) noexcept : buckets_(buckets) {
    UASSERT(buckets);
  }

  /*implicit*/ operator HistogramView() const noexcept {
    return Access::MakeView(buckets_);
  }

  template <typename InputRange>
  void SetBounds(const InputRange& upper_bounds) const {
    UINVARIANT(
        std::all_of(upper_bounds.begin(), upper_bounds.end(), IsBoundPositive),
        "Histogram bounds must be positive");
    UINVARIANT(std::is_sorted(upper_bounds.begin(), upper_bounds.end()),
               "Histogram bounds must be sorted");
    UINVARIANT(std::adjacent_find(upper_bounds.begin(), upper_bounds.end()) ==
                   upper_bounds.end(),
               "Histogram bounds must not contain duplicates");
    if (std::size(upper_bounds) != 0) {
      UINVARIANT(*upper_bounds.begin(), "Histogram bounds must be positive");
    }
    buckets_[0].upper_bound.size = std::size(upper_bounds);
    boost::copy(upper_bounds, Access::Bounds(*this).begin());
  }

  // Atomic for 'other', non-atomic for 'this'
  void Assign(HistogramView other) const noexcept {
    buckets_[0].upper_bound.size = other.GetBucketCount();
    buckets_[0].counter.store(other.GetValueAtInf(), std::memory_order_relaxed);
    boost::copy(Access::Buckets(other), Access::Buckets(*this).begin());
  }

  // Atomic
  void Account(double value, std::uint64_t count) const noexcept {
    const auto bounds_view = Access::Bounds(*this);
    const auto iter =
        boost::upper_bound(bounds_view, value, std::less_equal<>{});
    auto& bucket = (iter == bounds_view.end()) ? buckets_[0] : *iter.base();
    bucket.counter.fetch_add(count, std::memory_order_relaxed);
  }

  // Atomic
  void Reset() const noexcept {
    buckets_[0].counter.store(0, std::memory_order_relaxed);
    for (auto& bucket : Access::Buckets(*this)) {
      bucket.counter.store(0, std::memory_order_relaxed);
    }
  }

  // Non-atomic
  void Add(HistogramView other) const {
    UINVARIANT(HistogramView{*this}.GetBucketCount() == other.GetBucketCount(),
               "Mismatch in added Histogram sizes");
    UASSERT_MSG(HasSameBounds(*this, other),
                "Mismatch in added Histogram bounds");
    AddNonAtomic(buckets_[0].counter, other.GetValueAtInf());
    for (const auto tuple_of_refs :
         boost::combine(Access::Buckets(*this), Access::Values(other))) {
      auto& self_bucket = boost::get<0>(tuple_of_refs);
      const auto other_value = boost::get<1>(tuple_of_refs);
      AddNonAtomic(self_bucket.counter, other_value);
    }
  }

 private:
  friend struct Access;

  Bucket* buckets_;
};

}  // namespace utils::statistics::impl::histogram

USERVER_NAMESPACE_END
