#include <utest/utest.hpp>

#include <utils/mock_now.hpp>
#include <utils/token_bucket.hpp>

TEST(TokenBucket, Obtain) {
  utils::TokenBucket tb(1, std::chrono::seconds(1));
  EXPECT_TRUE(tb.Obtain());
  EXPECT_FALSE(tb.Obtain());
}

TEST(TokenBucket, Update) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  utils::TokenBucket tb(3, std::chrono::seconds(1));

  EXPECT_TRUE(tb.Obtain());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_FALSE(tb.Obtain());

  utils::datetime::MockSleep(std::chrono::seconds(2));
  EXPECT_TRUE(tb.Obtain());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_FALSE(tb.Obtain());
}

TEST(TokenBucket, UpdateFractional) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  utils::TokenBucket tb(1, std::chrono::seconds(2));

  EXPECT_TRUE(tb.Obtain());
  EXPECT_FALSE(tb.Obtain());

  utils::datetime::MockSleep(std::chrono::seconds(1));
  EXPECT_FALSE(tb.Obtain());

  utils::datetime::MockSleep(std::chrono::seconds(1));
  EXPECT_TRUE(tb.Obtain());
  EXPECT_FALSE(tb.Obtain());
}

TEST(TokenBucket, UpdateAfterLongSleep) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  utils::TokenBucket tb(2, std::chrono::seconds(2));

  EXPECT_TRUE(tb.Obtain());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_FALSE(tb.Obtain());

  utils::datetime::MockSleep(std::chrono::seconds(100));
  EXPECT_TRUE(tb.Obtain());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_FALSE(tb.Obtain());
}

TEST(TokenBucket, NoLimit) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  utils::TokenBucket tb(1, std::chrono::seconds(0));

  for (int i = 0; i < 1000; i++) {
    EXPECT_TRUE(tb.Obtain());
  }
}
