#include <ydb/impl/native_metrics.hpp>

#include <ydb-cpp-sdk/library/monlib/metrics/metric_consumer.h>

#include <limits>

#include <boost/container/small_vector.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/histogram_aggregator.hpp>
#include <userver/utils/statistics/histogram_view.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/metric_value.hpp>
#include <userver/ydb/impl/string_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {
namespace {

class WriterMetricConsumer final : public NMonitoring::IMetricConsumer {
 public:
  explicit WriterMetricConsumer(utils::statistics::Writer& writer)
      : writer_(writer) {}

  void OnStreamBegin() override {}
  void OnStreamEnd() override {}

  void OnCommonTime(TInstant /*time*/) override {}

  void OnMetricBegin(NMonitoring::EMetricType type) override {
    metric_type_.reset();
    labels_storage_.clear();
    labels_.clear();
    labels_written_ = false;
    sensor_.clear();
    sensor_written_ = false;
    histogram_storage_.reset();
    value_.reset();

    metric_type_.emplace(type);
  }

  void OnMetricEnd() override {
    UINVARIANT(metric_type_ && labels_written_ && value_, "Broken YDB metric");
    const auto metric_value = MakeMetricValue(*metric_type_, *value_);
    // To detect multiple OnMetricEnd without OnMetricBegin.
    metric_type_.reset();
    if (!metric_value) {
      // Skip an unsupported / broken metric.
      return;
    }
    if (!sensor_written_) {
      // Skip a metric without 'sensor' label.
      return;
    }

    writer_[sensor_].ValueWithLabels(*metric_value,
                                     utils::statistics::LabelsSpan{labels_});
  }

  void OnLabelsBegin() override { labels_written_ = true; }

  void OnLabelsEnd() override {}

  void OnLabel(impl::StringView name, impl::StringView value) override {
    if (name == impl::StringView{"sensor"}) {
      sensor_ = value;
      sensor_written_ = true;
      return;
    }

    const bool label_refs_invalidated =
        labels_storage_.size() == labels_storage_.capacity();
    auto& stored_label =
        labels_storage_.emplace_back(std::string{name}, std::string{value});

    if (label_refs_invalidated) {
      labels_.clear();
      for (const auto& label : labels_storage_) {
        labels_.emplace_back(label);
      }
    } else {
      labels_.emplace_back(stored_label);
    }
  }

  void OnLabel(ui32 name, ui32 value) override {
    UASSERT(name < prepared_labels_.size());
    UASSERT(value < prepared_labels_.size());
    OnLabel(prepared_labels_[name], prepared_labels_[value]);
  }

  std::pair<ui32, ui32> PrepareLabel(impl::StringView name,
                                     impl::StringView value) override {
    const auto old_size = prepared_labels_.size();
    prepared_labels_.emplace_back(name);
    prepared_labels_.emplace_back(value);
    return {old_size, old_size + 1};
  }

  void OnDouble(TInstant /*time*/, double value) override {
    UASSERT(!value_);
    value_.emplace(value);
  }

  void OnInt64(TInstant /*time*/, i64 value) override {
    UASSERT(!value_);
    value_.emplace(value);
  }

  void OnUint64(TInstant /*time*/, ui64 value) override {
    UASSERT(!value_);
    value_.emplace(value);
  }

  void OnHistogram(TInstant /*time*/,
                   NMonitoring::IHistogramSnapshotPtr snapshot) override {
    const auto count = snapshot->Count();
    UASSERT(count >= 1);
    UINVARIANT(
        snapshot->UpperBound(count - 1) >= std::numeric_limits<double>::max(),
        "Largest histogram bucket should be inf");
    bounds_storage_.clear();
    bounds_storage_.resize(count - 1);
    for (std::size_t i = 0; i < count - 1; ++i) {
      bounds_storage_[i] = snapshot->UpperBound(i);
    }
    histogram_storage_.emplace(bounds_storage_);
    for (std::size_t i = 0; i < count - 1; ++i) {
      histogram_storage_->AccountAt(i, snapshot->Value(i));
    }
    histogram_storage_->AccountInf(snapshot->Value(count - 1));
    value_.emplace(histogram_storage_->GetView());
  }

  void OnLogHistogram(
      TInstant /*time*/,
      NMonitoring::TLogHistogramSnapshotPtr /*snapshot*/) override {
    UASSERT(!value_);
    value_.emplace(Unsupported{});
  }

  void OnSummaryDouble(
      TInstant /*time*/,
      NMonitoring::ISummaryDoubleSnapshotPtr /*snapshot*/) override {
    UASSERT(!value_);
    value_.emplace(Unsupported{});
  }

 private:
  struct Unsupported {};

  using RawMetricValue =
      std::variant<double, std::int64_t, std::uint64_t,
                   utils::statistics::HistogramView, Unsupported>;

  static constexpr std::size_t kLabelsSsoSize = 8;
  static constexpr std::size_t kBucketsSsoSize = 50;

  std::optional<utils::statistics::MetricValue> MakeMetricValue(
      NMonitoring::EMetricType type, RawMetricValue value) {
    using Result = std::optional<utils::statistics::MetricValue>;
    switch (type) {
      case NMonitoring::EMetricType::GAUGE:
        if (const auto* const unwrapped_value = std::get_if<double>(&value)) {
          return Result{std::in_place, *unwrapped_value};
        }
        break;
      case NMonitoring::EMetricType::COUNTER:
        if (const auto* const unwrapped_value =
                std::get_if<std::int64_t>(&value)) {
          return Result{std::in_place, *unwrapped_value};
        }
        break;
      case NMonitoring::EMetricType::RATE:
        if (const auto* const unwrapped_value =
                std::get_if<std::uint64_t>(&value)) {
          return Result{std::in_place,
                        utils::statistics::Rate{*unwrapped_value}};
        }
        break;
      case NMonitoring::EMetricType::IGAUGE:
        if (const auto* const unwrapped_value =
                std::get_if<std::int64_t>(&value)) {
          return Result{std::in_place, *unwrapped_value};
        }
        break;
      case NMonitoring::EMetricType::HIST_RATE:
        if (const auto* const unwrapped_value =
                std::get_if<utils::statistics::HistogramView>(&value)) {
          return Result{std::in_place, *unwrapped_value};
        }
        break;
      default:
        break;
    }
    return std::nullopt;
  }

  utils::statistics::Writer& writer_;

  std::optional<NMonitoring::EMetricType> metric_type_;
  boost::container::small_vector<utils::statistics::Label, kLabelsSsoSize>
      labels_storage_;
  boost::container::small_vector<utils::statistics::LabelView, kLabelsSsoSize>
      labels_;
  bool labels_written_{false};
  std::string sensor_;
  bool sensor_written_{false};
  boost::container::small_vector<double, kBucketsSsoSize> bounds_storage_;
  std::optional<utils::statistics::HistogramAggregator> histogram_storage_;
  std::optional<RawMetricValue> value_;
  std::vector<std::string> prepared_labels_;
};

}  // namespace
}  // namespace ydb::impl

namespace utils::statistics {

void DumpMetric(Writer& writer,
                const NMonitoring::IMetricSupplier& metric_supplier) {
  ydb::impl::WriterMetricConsumer consumer{writer};
  metric_supplier.Append(TInstant::Now(), &consumer);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
