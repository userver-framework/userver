#include <userver/utils/statistics/graphite.hpp>

#include <algorithm>
#include <iterator>
#include <unordered_map>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/utils/algo.hpp>
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

std::string MakeCommonLabelsString(
    const std::unordered_map<std::string, std::string>& common_labels) {
  fmt::memory_buffer buf;

  for (const auto& [key, value] : common_labels) {
    buf.push_back(';');

    AppendGraphiteSafe(buf, key);
    buf.push_back('=');
    AppendGraphiteSafe(buf, value);
  }

  return to_string(buf);
}

class FormatBuilder final : public utils::statistics::BaseExposeFormatBuilder {
 public:
  explicit FormatBuilder(
      const std::unordered_map<std::string, std::string>& common_labels)
      : ending_(
            fmt::format(FMT_COMPILE(" {}\n"),
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count())),
        common_labels_(MakeCommonLabelsString(common_labels)) {}

  void HandleMetric(std::string_view path,
                    const std::vector<utils::statistics::Label>& labels,
                    const MetricValue& value) override {
    AppendGraphiteSafe(buf_, path);

    for (const auto& label : labels) {
      PutLabel(label);
    }

    buf_.append(common_labels_);

    std::visit(
        [this](const auto& v) {
          fmt::format_to(std::back_inserter(buf_), FMT_COMPILE(" {}"), v);
        },
        value);
    buf_.append(ending_);
  }

  std::string Release() { return fmt::to_string(buf_); }

 private:
  void PutLabel(const Label& label) {
    buf_.push_back(';');

    AppendGraphiteSafe(buf_, label.Name());
    buf_.push_back('=');
    AppendGraphiteSafe(buf_, label.Value());
  }

  const std::string ending_;
  const std::string common_labels_;
  fmt::memory_buffer buf_;
  std::unordered_map<std::string, std::string> metrics_;
};

}  // namespace

std::string ToGraphiteFormat(
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::Storage& statistics,
    const utils::statistics::StatisticsRequest& statistics_request) {
  FormatBuilder builder{common_labels};
  statistics.VisitMetrics(builder, statistics_request);
  return builder.Release();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
