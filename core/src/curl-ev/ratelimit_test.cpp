#include <userver/utest/utest.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include <curl-ev/ratelimit.hpp>
#include <userver/utils/mock_now.hpp>

// N.B.: These tests must pass without RunStandalone

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kGlobalSocketLimit =
    curl::errc::RateLimitErrorCode::kGlobalSocketLimit;
constexpr auto kPerHostSocketLimit =
    curl::errc::RateLimitErrorCode::kPerHostSocketLimit;

std::error_code GetCheckError(curl::ConnectRateLimiter& limiter,
                              const char* url) {
  std::error_code ec;
  limiter.Check(url, ec);
  return ec;
}

}  // namespace

TEST(CurlConnectRateLimiter, NoLimit) {
  constexpr size_t kRepetitions = 10000;
  utils::datetime::MockNowSet({});

  curl::ConnectRateLimiter limiter;
  for (size_t i = 0; i < kRepetitions; ++i) {
    ASSERT_FALSE(GetCheckError(limiter, "http://test"));
  }
}

TEST(CurlConnectRateLimiter, GlobalLimits) {
  constexpr size_t kRepetitions = 100;
  constexpr std::chrono::seconds kAcquireInterval{2};
  utils::datetime::MockNowSet({});

  curl::ConnectRateLimiter limiter;
  limiter.SetGlobalHttpLimits(2, kAcquireInterval);
  limiter.SetGlobalHttpsLimits(3, kAcquireInterval / 2);

  // extra connections from the limit
  EXPECT_FALSE(GetCheckError(limiter, "http://init"));
  EXPECT_FALSE(GetCheckError(limiter, "https://secure_init"));
  for (size_t i = 0; i < kRepetitions; ++i) {
    EXPECT_FALSE(GetCheckError(limiter, "http://init"));
    EXPECT_EQ(kGlobalSocketLimit, GetCheckError(limiter, "http://test"));
    EXPECT_EQ(kGlobalSocketLimit, GetCheckError(limiter, "http://init"));
    EXPECT_FALSE(GetCheckError(limiter, "https://secure_init"));
    EXPECT_FALSE(GetCheckError(limiter, "https://secure_init"));
    EXPECT_EQ(kGlobalSocketLimit,
              GetCheckError(limiter, "https://secure_init"));
    EXPECT_EQ(kGlobalSocketLimit,
              GetCheckError(limiter, "https://secure_test"));
    utils::datetime::MockSleep(kAcquireInterval);
  }
}

TEST(CurlConnectRateLimiter, PerHostLimits) {
  constexpr size_t kRepetitions = 100;
  constexpr std::chrono::seconds kAcquireInterval{1};
  utils::datetime::MockNowSet({});

  curl::ConnectRateLimiter limiter;
  limiter.SetPerHostLimits(2, kAcquireInterval);

  // extra connections from the limit
  EXPECT_FALSE(GetCheckError(limiter, "http://first"));
  EXPECT_FALSE(GetCheckError(limiter, "http://second"));
  EXPECT_FALSE(GetCheckError(limiter, "https://third"));
  for (size_t i = 0; i < kRepetitions; ++i) {
    EXPECT_FALSE(GetCheckError(limiter, "http://first"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "http://first"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "https://first"));
    EXPECT_FALSE(GetCheckError(limiter, "https://second"));
    EXPECT_FALSE(GetCheckError(limiter, "http://third"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "http://second"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "https://second"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "http://third"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "https://third"));
    utils::datetime::MockSleep(kAcquireInterval);
  }
}

TEST(CurlConnectRateLimiter, AllLimits) {
  constexpr size_t kRepetitions = 2;
  constexpr std::chrono::seconds kAcquireInterval{6};
  utils::datetime::MockNowSet({});

  curl::ConnectRateLimiter limiter;
  limiter.SetPerHostLimits(1, kAcquireInterval);
  limiter.SetGlobalHttpLimits(2, kAcquireInterval / 2);
  limiter.SetGlobalHttpsLimits(3, kAcquireInterval / 3);

  for (size_t i = 0; i < kRepetitions; ++i) {
    EXPECT_FALSE(GetCheckError(limiter, "http://first"));
    EXPECT_FALSE(GetCheckError(limiter, "http://second"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "http://first"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "http://second"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "https://first"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "https://second"));

    EXPECT_FALSE(GetCheckError(limiter, "https://secure_first"));
    EXPECT_EQ(kPerHostSocketLimit,
              GetCheckError(limiter, "https://secure_first"));
    EXPECT_FALSE(GetCheckError(limiter, "https://secure_second"));
    EXPECT_FALSE(GetCheckError(limiter, "https://secure_third"));
    EXPECT_EQ(kPerHostSocketLimit,
              GetCheckError(limiter, "https://secure_second"));
    EXPECT_EQ(kPerHostSocketLimit,
              GetCheckError(limiter, "https://secure_third"));
    EXPECT_EQ(kPerHostSocketLimit,
              GetCheckError(limiter, "http://secure_first"));
    EXPECT_EQ(kPerHostSocketLimit,
              GetCheckError(limiter, "http://secure_second"));
    EXPECT_EQ(kPerHostSocketLimit,
              GetCheckError(limiter, "http://secure_third"));

    EXPECT_EQ(kGlobalSocketLimit, GetCheckError(limiter, "http://fourth"));
    EXPECT_EQ(kPerHostSocketLimit, GetCheckError(limiter, "https://fourth"));
    utils::datetime::MockSleep(kAcquireInterval);
  }
}

// asserts in debug
#ifdef NDEBUG
TEST(CurlConnectRateLimiter, InvalidUrlFallback) {
  constexpr std::chrono::seconds kAcquireInterval{1};
  utils::datetime::MockNowSet({});

  curl::ConnectRateLimiter limiter;
  limiter.SetGlobalHttpLimits(1, kAcquireInterval);

  EXPECT_FALSE(GetCheckError(limiter, nullptr));
  EXPECT_TRUE(GetCheckError(limiter, nullptr));
  utils::datetime::MockSleep(kAcquireInterval);
  EXPECT_FALSE(GetCheckError(limiter, nullptr));

  EXPECT_TRUE(GetCheckError(limiter, ""));
  utils::datetime::MockSleep(kAcquireInterval);
  EXPECT_FALSE(GetCheckError(limiter, ""));

  EXPECT_TRUE(GetCheckError(limiter, "no-schema"));
  utils::datetime::MockSleep(kAcquireInterval);
  EXPECT_FALSE(GetCheckError(limiter, "no-schema"));

  EXPECT_TRUE(GetCheckError(limiter, "http://"));
  utils::datetime::MockSleep(kAcquireInterval);
  EXPECT_FALSE(GetCheckError(limiter, "http://"));

  EXPECT_TRUE(GetCheckError(limiter, "http://test"));
  EXPECT_FALSE(GetCheckError(limiter, "https://test"));
}
#endif

TEST(CurlConnectRateLimiter, Multithread) {
  constexpr size_t kThreadsCount = 4;
  constexpr size_t kPerHostLimit = 10000;
  utils::datetime::MockNowSet({});

  curl::ConnectRateLimiter limiter;
  limiter.SetGlobalHttpLimits(kPerHostLimit * kThreadsCount,
                              std::chrono::seconds{1});
  limiter.SetPerHostLimits(kPerHostLimit, std::chrono::seconds{1});

  std::atomic<bool> has_started{false};
  std::vector<std::string> urls;
  std::vector<std::future<std::pair<size_t, size_t>>> workers;
  for (size_t worker_idx = 0; worker_idx < kThreadsCount; ++worker_idx) {
    workers.push_back(std::async(
        std::launch::async,
        [worker_idx, &has_started, &urls](
            curl::ConnectRateLimiter& limiter) -> std::pair<size_t, size_t> {
          while (!has_started) std::this_thread::yield();

          size_t acquired_ours = 0;
          size_t acquired_theirs = 0;
          while (!GetCheckError(limiter, urls[worker_idx].c_str())) {
            ++acquired_ours;
            if (!GetCheckError(limiter, urls[worker_idx + 1].c_str())) {
              ++acquired_theirs;
            }
          }
          return {acquired_ours, acquired_theirs};
        },
        std::ref(limiter)));
    urls.push_back("http://url-" + std::to_string(worker_idx));
  }
  urls.push_back(urls.front());
  ASSERT_EQ(urls.size(), workers.size() + 1);

  has_started = true;
  std::vector<size_t> acquired_totals(workers.size());
  for (size_t i = 0; i < workers.size(); ++i) {
    auto [ours, theirs] = workers[i].get();
    acquired_totals[i] += ours;
    acquired_totals[(i + 1) % acquired_totals.size()] += theirs;
  }

  for (auto acquired_total : acquired_totals) {
    EXPECT_EQ(acquired_total, kPerHostLimit);
  }
}

USERVER_NAMESPACE_END
