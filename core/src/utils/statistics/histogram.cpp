#include <userver/utils/statistics/histogram.hpp>

#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif

#include <cmath>

#include <userver/utils/statistics/impl/histogram_bucket.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <utils/statistics/impl/histogram_view_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

// We use a B+ tree to accelerate Account in histograms with "small" number
// of buckets. In a binary search tree, 1 value is stored in each node. In a B+
// tree, kBlockSize values are stored in each node (block). They are compared
// with a value all at once using SIMD.
constexpr std::size_t kBlockSize = 8;
// After comparing a value with elements in a bounds block, there are
// the following possible outcomes:
// - value < block[0]
// - block[0] < block < block[1]
// - ...
// - block[kBlockSize-1] < value
constexpr std::size_t kBlockWays = kBlockSize + 1;
// Each way from the previous block leads to a block from the next layer.
constexpr std::size_t kBlockLayers = 2;
// kBlockWays^0 from 1st layer + kBlockWays^1 from 2nd layer
constexpr std::size_t kBlocksCount = 1 + kBlockWays;
// The maximum number of bounds that fits in the B+ tree with our parameters.
constexpr std::size_t kMaxBPlusBounds = kBlocksCount * kBlockSize;
static_assert(kMaxBPlusBounds >= 50,
              "B+ tree should fit the largest recommended histogram size");
// For demonstration purposes only. Fix the constant if the assert fails.
static_assert(kMaxBPlusBounds == 80);

namespace impl::histogram {

struct alignas(sizeof(float) * kBlockSize) BoundsBlock final {
  float data[kBlockSize]{};
};

}  // namespace impl::histogram

using impl::histogram::BoundsBlock;

namespace {

#ifdef __clang__
#define USERVER_IMPL_ALWAYS_INLINE_SIMD __attribute__((always_inline))
#else
#define USERVER_IMPL_ALWAYS_INLINE_SIMD
#endif

// Returns kBlockSize if `value` is greater than all of `block`.
USERVER_IMPL_ALWAYS_INLINE_SIMD std::size_t LeastGreaterEqualIndex(
    const BoundsBlock& block, float value) noexcept {
#if defined(__AVX2__)
  static constexpr int kLessEqual = 2;
  const auto mask = _mm256_movemask_ps(_mm256_cmp_ps(
      _mm256_set1_ps(value), _mm256_load_ps(&block.data[0]), kLessEqual));
#elif defined(__SSE2__)
  const auto mask1 = _mm_movemask_ps(
      _mm_cmple_ps(_mm_set1_ps(value), _mm_load_ps(&block.data[0])));
  const auto mask2 = _mm_movemask_ps(
      _mm_cmple_ps(_mm_set1_ps(value), _mm_load_ps(&block.data[4])));
  const auto mask = mask1 | (mask2 << 4);
#else
  std::uint32_t mask = 0;
  for (std::size_t i = 0; i < kBlockSize; ++i) {
    mask |= ((static_cast<float>(value) <= block.data[i]) << i);
  }
#endif
  return __builtin_ctz(mask | (1 << kBlockSize));
}

}  // namespace

void Histogram::UpdateBounds() {
  if (bucket_count_ > kMaxBPlusBounds) return;

  const auto upper_bounds = impl::histogram::Access::Bounds(GetView());
  for (const auto bound : upper_bounds) {
    UINVARIANT(std::isnormal(bound), "Histogram bounds must fit in 'float'");
  }

  const auto largest_bound =
      upper_bounds.empty() ? 0.0f
                           : upper_bounds.begin()[upper_bounds.size() - 1];
  const auto get_upper_bound = [&](std::size_t i) {
    return i >= upper_bounds.size() ? largest_bound : upper_bounds.begin()[i];
  };
  bounds_ = std::make_unique<impl::histogram::BoundsBlock[]>(kBlocksCount);
  for (std::size_t i = 0; i < kBlockSize; ++i) {
    bounds_[0].data[i] = get_upper_bound((i + 1) * kBlockWays - 1);
  }
  for (std::size_t i = 0; i < kBlockWays; ++i) {
    for (std::size_t j = 0; j < kBlockSize; ++j) {
      bounds_[1 + i].data[j] = get_upper_bound(i * kBlockWays + (j + 1) - 1);
    }
  }
}

Histogram::Histogram(utils::span<const double> upper_bounds)
    : buckets_(
          std::make_unique<impl::histogram::Bucket[]>(upper_bounds.size() + 1)),
      bucket_count_(upper_bounds.size()) {
  impl::histogram::CopyBounds(buckets_.get(), upper_bounds);
  UpdateBounds();
}

Histogram::Histogram(HistogramView other)
    : buckets_(std::make_unique<impl::histogram::Bucket[]>(
          other.GetBucketCount() + 1)),
      bucket_count_(other.GetBucketCount()) {
  impl::histogram::CopyBoundsAndValues(buckets_.get(), other);
  UpdateBounds();
}

Histogram::Histogram(Histogram&& other) noexcept = default;

Histogram& Histogram::operator=(Histogram&& other) noexcept = default;

Histogram::Histogram(const Histogram& other) : Histogram(other.GetView()) {}

Histogram& Histogram::operator=(const Histogram& other) {
  *this = Histogram{other};
  return *this;
}

Histogram::~Histogram() = default;

// NOLINTNEXTLINE(readability-make-member-function-const)
void Histogram::Account(double value, std::uint64_t count) noexcept {
  if (bucket_count_ > kMaxBPlusBounds) {
    impl::histogram::Account(buckets_.get(), value, count);
    return;
  }

  std::size_t block_index = 0;
  for (std::size_t i = 0; i < kBlockLayers; ++i) {
    block_index = 1 + block_index * kBlockWays +
                  LeastGreaterEqualIndex(bounds_[block_index], value);
  }
  // block_index now points to a block in a hypothetical additional layer.
  const auto pre_bucket_index = block_index - kBlocksCount;
  // 0th bucket is the "infinity" bucket.
  const auto bucket_index =
      pre_bucket_index + 1 > bucket_count_ ? 0 : pre_bucket_index + 1;
  auto& bucket = buckets_[bucket_index];
  bucket.counter.fetch_add(count, std::memory_order_relaxed);
}

void ResetMetric(Histogram& histogram) noexcept {
  impl::histogram::ResetMetric(histogram.buckets_.get());
}

HistogramView Histogram::GetView() const& noexcept {
  return impl::histogram::MakeView(buckets_.get());
}

void DumpMetric(Writer& writer, const Histogram& histogram) {
  writer = histogram.GetView();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
