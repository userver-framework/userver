#include <userver/utils/statistics/visit.hpp>

#include <iterator>
#include <string>
#include <string_view>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/metric_value.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

/// [VisitMetrics custom format]
class MetricsMessageBuilder final : public utils::statistics::BaseFormatBuilder {
public:
    void HandleMetric(
        std::string_view path,
        utils::statistics::LabelsSpan labels,
        const utils::statistics::MetricValue& value
    ) override {
        fmt::format_to(std::back_inserter(message_), "{}\t{}\t{}\n", path, labels, value);
    }

    const std::string& GetMessage() const { return message_; }

private:
    std::string message_;
};
/// [VisitMetrics custom format]

}  // namespace

TEST(MetricsWriter, VisitMetricsCustomFormat) {
    /// [VisitMetrics custom format usage]
    utils::statistics::RateCounter requests{10};
    utils::statistics::RateCounter errors{2};
    MetricsMessageBuilder builder;

    utils::statistics::VisitMetrics(
        [&](utils::statistics::Writer& writer) {
            writer["requests"] = requests;
            writer["errors"].ValueWithLabels(errors, {"kind", "timeout"});
        },
        builder
    );

    EXPECT_EQ(builder.GetMessage(), "requests\t\t10\nerrors\tkind=timeout\t2\n");
    /// [VisitMetrics custom format usage]
}

USERVER_NAMESPACE_END
