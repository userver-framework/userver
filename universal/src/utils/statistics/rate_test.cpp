#include <userver/utils/statistics/rate.hpp>

#include <iterator>
#include <string>
#include <type_traits>

// Intentionally omit <fmt/format.h> to verify that the tested header
// supplies all dependencies required by its formatters.
#include <fmt/core.h>
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

static_assert(
    std::is_trivially_copy_constructible_v<Rate>,
    "Rate is widely used in engine internals. It should be trivially "
    "copy constructible to be returned in CPU registers."
);

TEST(Rate, Basic) {
    EXPECT_EQ(Rate{5}, Rate{2} + Rate{3});

    {
        Rate test1{5};
        test1 += Rate{10};
        EXPECT_EQ(Rate{15}, test1);
    }
}

TEST(Rate, Fmt) {
    std::string result;
    fmt::format_to(std::back_inserter(result), "{}", Rate{10});
    EXPECT_EQ("10", result);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
