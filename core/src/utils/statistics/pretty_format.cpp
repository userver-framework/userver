#include <userver/utils/statistics/pretty_format.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>

#include <fmt/compile.h>
#include <fmt/format.h>
#include <boost/container/small_vector.hpp>

#include <userver/utils/overloaded.hpp>
#include <userver/utils/statistics/fmt.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

class FormatBuilder final : public utils::statistics::BaseFormatBuilder {
 public:
  FormatBuilder() = default;

  void HandleMetric(std::string_view path, utils::statistics::LabelsSpan labels,
                    const MetricValue& value) override {
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE("{}:"), path);

    constexpr std::size_t kInitialVectorSize = 16;
    boost::container::small_vector<std::size_t, kInitialVectorSize>
        sorted_labels_indexes(labels.size());
    std::iota(sorted_labels_indexes.begin(), sorted_labels_indexes.end(), 0);
    std::sort(sorted_labels_indexes.begin(), sorted_labels_indexes.end(),
              [&labels](const auto& lhs, const auto& rhs) {
                return (labels.begin() + lhs)->Name() <
                       (labels.begin() + rhs)->Name();
              });

    for (std::size_t i = 0; i < sorted_labels_indexes.size(); ++i) {
      PutLabel(*(labels.begin() + sorted_labels_indexes[i]));
      if (i + 1 < sorted_labels_indexes.size()) {
        PutSeparator();
      }
    }

    const auto type = value.Visit(utils::Overloaded{
        [](const Rate&) -> std::string_view { return "RATE"; },
        [](const auto&) -> std::string_view { return "GAUGE"; }});
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE("\t{}\t{}\n"), type,
                   value);
  }

  std::string Release() { return fmt::to_string(buf_); }

 private:
  void PutLabel(const LabelView& label) {
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE(" {}={}"),
                   label.Name(), label.Value());
  }

  void PutSeparator() {
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE(","));
  }

  fmt::memory_buffer buf_;
};

}  // namespace

std::string ToPrettyFormat(const utils::statistics::Storage& statistics,
                           const utils::statistics::Request& request) {
  FormatBuilder builder;
  statistics.VisitMetrics(builder, request);
  return std::move(builder).Release();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
