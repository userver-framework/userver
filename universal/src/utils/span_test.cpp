#include <userver/utils/span.hpp>

#include <numeric>
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
  static_assert(
      std::is_convertible_v<std::vector<int>&&, utils::span<const int>>);
}

TEST(UtilsSpan, BoostAdapters) {
  const int array[] = {1, 2, 3};
  const utils::span span = array;
  const auto transformed =
      span | boost::adaptors::transformed([](int x) { return x * 2; });
  EXPECT_THAT(transformed, testing::ElementsAre(2, 4, 6));
}

USERVER_NAMESPACE_END
