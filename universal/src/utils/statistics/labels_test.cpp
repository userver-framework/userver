#include <userver/utils/statistics/labels.hpp>

#include <compare>
#include <iterator>
#include <string>

// Intentionally omit <fmt/format.h> to verify that the tested header
// supplies all dependencies required by its formatters.
#include <fmt/core.h>
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

TEST(Label, OrderIsNameThenValue) {
    const Label handler_late{"handler", "z"};
    const Label query_early{"query", "a"};
    const Label handler_list{"handler", "/list"};
    const Label handler_orders{"handler", "/orders"};

    EXPECT_LT(handler_late, query_early);
    EXPECT_LT(handler_list, handler_orders);
    EXPECT_EQ(handler_list <=> handler_list, std::strong_ordering::equal);
}

TEST(LabelView, OrderIsNameThenValue) {
    const LabelView handler_late{"handler", "z"};
    const LabelView query_early{"query", "a"};
    const LabelView handler_list{"handler", "/list"};
    const LabelView handler_orders{"handler", "/orders"};

    EXPECT_LT(handler_late, query_early);
    EXPECT_LT(handler_list, handler_orders);
    EXPECT_EQ(handler_late <=> query_early, std::strong_ordering::less);
    EXPECT_EQ(handler_list <=> handler_list, std::strong_ordering::equal);
}

TEST(Label, Fmt) {
    const Label label{"method", "GET"};
    std::string result;
    fmt::format_to(std::back_inserter(result), "{}", label);
    EXPECT_EQ("method=GET", result);
}

TEST(LabelView, Fmt) {
    const LabelView label{"kind", "timeout"};
    std::string result;
    fmt::format_to(std::back_inserter(result), "{}", label);
    EXPECT_EQ("kind=timeout", result);
}

TEST(LabelsSpan, Fmt) {
    const LabelView labels[] = {{"method", "GET"}, {"kind", "timeout"}};
    const LabelsSpan span{std::begin(labels), std::end(labels)};
    std::string result;
    fmt::format_to(std::back_inserter(result), "{}", span);
    EXPECT_EQ("method=GET;kind=timeout", result);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
