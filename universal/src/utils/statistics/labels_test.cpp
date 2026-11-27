#include <userver/utils/statistics/labels.hpp>

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

}  // namespace utils::statistics

USERVER_NAMESPACE_END
