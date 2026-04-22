#include <userver/utils/span.hpp>

#include <numeric>
#include <ranges>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/range/adaptor/transformed.hpp>

USERVER_NAMESPACE_BEGIN

TEST(UtilsSpan, NonConst) {
    std::vector<int> array{1, 2, 3};
    const utils::span span = array;
    EXPECT_EQ(std::accumulate(span.begin(), span.end(), 0), 6);
    EXPECT_EQ(span.data(), &*span.begin());
    EXPECT_EQ(span.size(), 3);
    EXPECT_EQ(span[1], 2);

    span[0] = 5;
    EXPECT_EQ(std::accumulate(span.begin(), span.end(), 0), 10);
}

TEST(UtilsSpan, Const) {
    const std::vector<int> array{1, 2, 3};
    const utils::span span = array;
    EXPECT_EQ(std::accumulate(span.begin(), span.end(), 0), 6);
    EXPECT_EQ(span.data(), &*span.begin());
    EXPECT_EQ(span.size(), 3);
    EXPECT_EQ(span[1], 2);
}

TEST(UtilsSpan, RvalueContainer) {
    static_assert(!std::is_convertible_v<std::vector<int>&&, utils::span<int>>);
    static_assert(std::is_convertible_v<std::vector<int>&&, utils::span<const int>>);
}

TEST(UtilsSpan, BoostAdapters) {
    const int array[] = {1, 2, 3};
    const utils::span span = array;
    const auto transformed = span | boost::adaptors::transformed([](int x) { return x * 2; });
    EXPECT_THAT(transformed, testing::ElementsAre(2, 4, 6));
}

TEST(UtilsSpan, DangerousInitializerListConversions) {
    // https://cplusplus.github.io/LWG/issue4520
    bool data[4] = {true, false, true, false};
    bool* ptr = data;
    std::size_t size = 4;

    {
        utils::span<const bool> span{ptr, size};
        EXPECT_THAT(span, testing::ElementsAre(true, false, true, false));
    }
    {
        utils::span span{ptr, size};
        static_assert(std::same_as<std::ranges::range_reference_t<decltype(span)>, bool&>);
        EXPECT_THAT(span, testing::ElementsAre(true, false, true, false));
    }
}

TEST(UtilsSpan, DangerousInitializerListConversionsInt) {
    // https://cplusplus.github.io/LWG/issue4520
    int data[4] = {1, 2, 3, 4};
    int* ptr = data;
    std::size_t size = 4;

    {
        utils::span<const int> span{ptr, size};
        EXPECT_THAT(span, testing::ElementsAre(1, 2, 3, 4));
    }
    {
        utils::span span{ptr, size};
        static_assert(std::same_as<std::ranges::range_reference_t<decltype(span)>, int&>);
        EXPECT_THAT(span, testing::ElementsAre(1, 2, 3, 4));
    }
}

TEST(UtilsSpan, DangerousInitializerListCTAD) {
    std::vector<int> array{1, 2, 3};

    {
        utils::span span{array};
        static_assert(std::same_as<std::ranges::range_reference_t<decltype(span)>, int&>);
        EXPECT_THAT(span, testing::ElementsAre(1, 2, 3));
    }
    {
        utils::span span{std::as_const(array)};
        static_assert(std::same_as<std::ranges::range_reference_t<decltype(span)>, const int&>);
        EXPECT_THAT(span, testing::ElementsAre(1, 2, 3));
    }
}

USERVER_NAMESPACE_END
