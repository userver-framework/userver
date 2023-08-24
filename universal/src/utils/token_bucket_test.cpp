#include <gtest/gtest.h>

#include <userver/utils/mock_now.hpp>
#include <userver/utils/token_bucket.hpp>

USERVER_NAMESPACE_BEGIN

TEST(TokenBucket, Default) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  utils::TokenBucket tb;
  EXPECT_EQ(0, tb.GetMaxSizeApprox());
  EXPECT_EQ(0, tb.GetTokensApprox());

  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());

  utils::datetime::MockSleep(std::chrono::seconds{100500});
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
}

TEST(TokenBucket, Obtain) {
  utils::TokenBucket tb{1, {1, std::chrono::seconds{1}}};
  EXPECT_EQ(1, tb.GetMaxSizeApprox());
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_EQ(1, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{1}, tb.GetRefillIntervalApprox());
  EXPECT_DOUBLE_EQ(1.0, tb.GetRatePs());

  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
}

TEST(TokenBucket, ObtainAll) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point{});

  utils::TokenBucket tb{10, {1, std::chrono::seconds{1}}};
  EXPECT_EQ(10, tb.GetMaxSizeApprox());
  EXPECT_EQ(10, tb.GetTokensApprox());
  EXPECT_EQ(1, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{1}, tb.GetRefillIntervalApprox());
  EXPECT_DOUBLE_EQ(1.0, tb.GetRatePs());

  EXPECT_TRUE(tb.ObtainAll(3));
  EXPECT_EQ(7, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(6, tb.GetTokensApprox());
  EXPECT_FALSE(tb.ObtainAll(9));
  EXPECT_EQ(6, tb.GetTokensApprox());
  EXPECT_TRUE(tb.ObtainAll(1));
  EXPECT_EQ(5, tb.GetTokensApprox());
  EXPECT_TRUE(tb.ObtainAll(0));
  EXPECT_EQ(5, tb.GetTokensApprox());

  utils::datetime::MockSleep(std::chrono::seconds{4});
  EXPECT_EQ(9, tb.GetTokensApprox());
  EXPECT_TRUE(tb.ObtainAll(1));
  EXPECT_EQ(8, tb.GetTokensApprox());
  EXPECT_TRUE(tb.ObtainAll(8));
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.ObtainAll(42));
  EXPECT_FALSE(tb.Obtain());
  EXPECT_TRUE(tb.ObtainAll(0));
  EXPECT_EQ(0, tb.GetTokensApprox());
}

TEST(TokenBucket, Refill) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  utils::TokenBucket tb{3, {1, std::chrono::seconds{1}}};
  EXPECT_EQ(3, tb.GetMaxSizeApprox());
  EXPECT_EQ(3, tb.GetTokensApprox());
  EXPECT_EQ(1, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{1}, tb.GetRefillIntervalApprox());
  EXPECT_DOUBLE_EQ(1.0, tb.GetRatePs());

  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(2, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());

  utils::datetime::MockSleep(std::chrono::seconds(2));
  EXPECT_EQ(2, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
}

TEST(TokenBucket, RefillFractional) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  utils::TokenBucket tb{1, {1, std::chrono::seconds{2}}};
  EXPECT_EQ(1, tb.GetMaxSizeApprox());
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_EQ(1, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{2}, tb.GetRefillIntervalApprox());
  EXPECT_DOUBLE_EQ(0.5, tb.GetRatePs());

  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());

  utils::datetime::MockSleep(std::chrono::seconds(1));
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());

  utils::datetime::MockSleep(std::chrono::seconds(1));
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
}

TEST(TokenBucket, RefillAfterLongSleep) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  utils::TokenBucket tb{2, {1, std::chrono::seconds{2}}};
  EXPECT_EQ(2, tb.GetMaxSizeApprox());
  EXPECT_EQ(2, tb.GetTokensApprox());
  EXPECT_EQ(1, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{2}, tb.GetRefillIntervalApprox());
  EXPECT_DOUBLE_EQ(0.5, tb.GetRatePs());

  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());

  utils::datetime::MockSleep(std::chrono::seconds(100));
  EXPECT_EQ(2, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
}

TEST(TokenBucket, NoLimit) {
  utils::datetime::MockNowSet(std::chrono::system_clock::time_point());

  auto tb = utils::TokenBucket::MakeUnbounded();
  tb.SetMaxSize(2);
  EXPECT_EQ(2, tb.GetMaxSizeApprox());
  EXPECT_EQ(2, tb.GetTokensApprox());

  for (int i = 0; i < 1000; i++) {
    EXPECT_TRUE(tb.Obtain());
    EXPECT_EQ(2, tb.GetTokensApprox());
    EXPECT_TRUE(tb.ObtainAll(2));
    EXPECT_EQ(2, tb.GetTokensApprox());
    EXPECT_FALSE(tb.ObtainAll(3));
    EXPECT_EQ(2, tb.GetTokensApprox());
  }
}

TEST(TokenBucket, Empty) {
  utils::TokenBucket tb{1, {1, std::chrono::seconds{0}}};
  EXPECT_EQ(1, tb.GetMaxSizeApprox());
  EXPECT_EQ(1, tb.GetTokensApprox());

  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(1, tb.GetTokensApprox());

  tb.SetMaxSize(0);
  EXPECT_EQ(0, tb.GetMaxSizeApprox());
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());
}

TEST(TokenBucket, Reconfig) {
  utils::datetime::MockNowSet({});

  utils::TokenBucket tb{10, {1, std::chrono::seconds{1}}};
  EXPECT_EQ(10, tb.GetMaxSizeApprox());
  EXPECT_EQ(10, tb.GetTokensApprox());
  EXPECT_EQ(1, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{1}, tb.GetRefillIntervalApprox());
  EXPECT_EQ(1.0, tb.GetRatePs());

  EXPECT_TRUE(tb.ObtainAll(4));
  EXPECT_EQ(6, tb.GetTokensApprox());

  tb.SetMaxSize(5);
  EXPECT_EQ(5, tb.GetMaxSizeApprox());
  EXPECT_EQ(5, tb.GetTokensApprox());
  EXPECT_TRUE(tb.ObtainAll(4));
  EXPECT_EQ(1, tb.GetTokensApprox());

  tb.SetInstantRefillPolicy();
  EXPECT_TRUE(tb.ObtainAll(5));
  EXPECT_FALSE(tb.ObtainAll(6));
  EXPECT_TRUE(tb.ObtainAll(5));

  tb.SetMaxSize(10);
  EXPECT_EQ(10, tb.GetMaxSizeApprox());
  EXPECT_TRUE(tb.ObtainAll(10));
  EXPECT_TRUE(tb.ObtainAll(10));

  utils::datetime::MockSleep(std::chrono::seconds{60});
  EXPECT_TRUE(tb.ObtainAll(10));
  EXPECT_TRUE(tb.ObtainAll(10));

  tb.SetRefillPolicy({1, std::chrono::seconds{1}});
  EXPECT_EQ(1, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{1}, tb.GetRefillIntervalApprox());
  EXPECT_EQ(10, tb.GetTokensApprox());
  EXPECT_TRUE(tb.ObtainAll(10));
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());

  tb.SetInstantRefillPolicy();
  EXPECT_TRUE(tb.ObtainAll(10));
}

TEST(TokenBucket, MultiTokenRefill) {
  utils::datetime::MockNowSet({});

  utils::TokenBucket tb{5, {1, std::chrono::seconds{1}}};
  EXPECT_EQ(5, tb.GetMaxSizeApprox());
  EXPECT_EQ(5, tb.GetTokensApprox());
  EXPECT_EQ(1, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{1}, tb.GetRefillIntervalApprox());
  EXPECT_EQ(1.0, tb.GetRatePs());

  EXPECT_TRUE(tb.ObtainAll(5));
  EXPECT_EQ(0, tb.GetTokensApprox());

  utils::datetime::MockSleep(std::chrono::seconds{2});
  EXPECT_EQ(2, tb.GetTokensApprox());
  EXPECT_TRUE(tb.ObtainAll(2));
  EXPECT_EQ(0, tb.GetTokensApprox());

  utils::datetime::MockSleep(std::chrono::seconds{1});
  EXPECT_EQ(1, tb.GetTokensApprox());
  EXPECT_TRUE(tb.Obtain());
  EXPECT_EQ(0, tb.GetTokensApprox());

  tb.SetRefillPolicy({2, std::chrono::seconds{2}});
  EXPECT_EQ(2, tb.GetRefillAmountApprox());
  EXPECT_EQ(std::chrono::seconds{2}, tb.GetRefillIntervalApprox());
  EXPECT_EQ(0, tb.GetTokensApprox());

  utils::datetime::MockSleep(std::chrono::seconds{2});
  EXPECT_EQ(2, tb.GetTokensApprox());
  EXPECT_TRUE(tb.ObtainAll(2));
  EXPECT_EQ(0, tb.GetTokensApprox());

  utils::datetime::MockSleep(std::chrono::seconds{1});
  EXPECT_EQ(0, tb.GetTokensApprox());
  EXPECT_FALSE(tb.Obtain());
}

USERVER_NAMESPACE_END
