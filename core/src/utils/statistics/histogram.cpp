#include <userver/utils/statistics/histogram.hpp>

#include <userver/utils/statistics/impl/histogram_bucket.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

Histogram::Histogram(utils::span<const double> upper_bounds)
    : buckets_(std::make_unique<impl::histogram::Bucket[]>(upper_bounds.size() +
                                                           1)) {
  impl::histogram::CopyBounds(buckets_.get(), upper_bounds);
}

Histogram::Histogram(HistogramView other)
    : buckets_(std::make_unique<impl::histogram::Bucket[]>(
          other.GetBucketCount() + 1)) {
  impl::histogram::CopyBoundsAndValues(buckets_.get(), other);
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
  impl::histogram::Account(buckets_.get(), value, count);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void Histogram::Add(HistogramView other) {
  impl::histogram::Add(buckets_.get(), other);
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
