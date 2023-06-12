#include <userver/utils/statistics/graphite.hpp>

#include <algorithm>
#include <iterator>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/utils/mock_now.hpp>
#include <userver/utils/statistics/fmt.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

bool IsGraphitePrintable(char c) noexcept {
  return std::isalnum(c) || std::strchr(".-_", c);
}

void AppendGraphiteSafe(fmt::memory_buffer& out, std::string_view value) {
  std::replace_copy_if(
      value.cbegin(), value.cend(), std::back_inserter(out),
      [](char c) { return !IsGraphitePrintable(c); }, '_');
}

class FormatBuilder final : public utils::statistics::BaseFormatBuilder {
 public:
  FormatBuilder()
      : ending_(fmt::format(FMT_COMPILE(" {}\n"),
                            std::chrono::duration_cast<std::chrono::seconds>(
                                utils::datetime::MockNow().time_since_epoch())
                                .count())) {}

  void HandleMetric(std::string_view path, utils::statistics::LabelsSpan labels,
                    const MetricValue& value) override {
    AppendGraphiteSafe(buf_, path);

    for (const auto& label : labels) {
      PutLabel(label);
    }

    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE(" {}"), value);
    buf_.append(ending_);
  }

  std::string Release() { return fmt::to_string(buf_); }

 private:
  void PutLabel(const LabelView& label) {
    buf_.push_back(';');

    AppendGraphiteSafe(buf_, label.Name());
    buf_.push_back('=');
    AppendGraphiteSafe(buf_, label.Value());
  }

  const std::string ending_;
  fmt::memory_buffer buf_;
};

}  // namespace

std::string ToGraphiteFormat(const utils::statistics::Storage& statistics,
                             const utils::statistics::Request& request) {
  FormatBuilder builder{};
  statistics.VisitMetrics(builder, request);
  return builder.Release();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
