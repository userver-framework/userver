#include <userver/utils/statistics/histogram_view.hpp>

#include <iterator>
#include <string>

// Intentionally omit <fmt/format.h> to verify that the tested header
// supplies all dependencies required by its formatters.
#include <fmt/core.h>
#include <gtest/gtest.h>

#include <userver/utils/statistics/histogram.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

TEST(HistogramView, Fmt) {
    Histogram histogram{{1.5, 5.0}};
    histogram.Account(1.0, 2);
    histogram.Account(9.0);
    const auto view = histogram.GetView();

    std::string result;
    fmt::format_to(std::back_inserter(result), "{}", view);
    EXPECT_EQ("[1.5]=2,[5]=0,[inf]=1", result);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
