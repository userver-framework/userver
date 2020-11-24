#include <utest/utest.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include <curl-ev/ratelimit.hpp>
#include <utils/mock_now.hpp>

// N.B.: These tests must pass without RunInCoro

namespace {
constexpr auto kOk = curl::RateLimitStatus::kOk;
constexpr auto kGlobalSocketLimit = curl::RateLimitStatus::kGlobalSocketLimit;
constexpr auto kPerHostSocketLimit = curl::RateLimitStatus::kPerHostSocketLimit;
}  // namespace

TEST(CurlConnectRateLimiter, NoLimit) {
  constexpr size_t kRepetitions = 10000;
  utils::datetime::MockNowSet({});

  curl::ConnectRateLimiter limiter;
  for (size_t i = 0; i < kRepetitions; ++i) {
    ASSERT_EQ(kOk, limiter.MayAcquireConnection("http://test"));
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
  EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://init"));
  EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://secure_init"));
  for (size_t i = 0; i < kRepetitions; ++i) {
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://init"));
    EXPECT_EQ(kGlobalSocketLimit, limiter.MayAcquireConnection("http://test"));
    EXPECT_EQ(kGlobalSocketLimit, limiter.MayAcquireConnection("http://init"));
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://secure_init"));
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://secure_init"));
    EXPECT_EQ(kGlobalSocketLimit,
              limiter.MayAcquireConnection("https://secure_init"));
    EXPECT_EQ(kGlobalSocketLimit,
              limiter.MayAcquireConnection("https://secure_test"));
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
  EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://first"));
  EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://second"));
  EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://third"));
  for (size_t i = 0; i < kRepetitions; ++i) {
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://first"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("http://first"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://first"));
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://second"));
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://third"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("http://second"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://second"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("http://third"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://third"));
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
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://first"));
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://second"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("http://first"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("http://second"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://first"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://second"));

    EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://secure_first"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://secure_first"));
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://secure_second"));
    EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://secure_third"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://secure_second"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://secure_third"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("http://secure_first"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("http://secure_second"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("http://secure_third"));

    EXPECT_EQ(kGlobalSocketLimit,
              limiter.MayAcquireConnection("http://fourth"));
    EXPECT_EQ(kPerHostSocketLimit,
              limiter.MayAcquireConnection("https://fourth"));
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

  EXPECT_EQ(kOk, limiter.MayAcquireConnection(nullptr));
  EXPECT_NE(kOk, limiter.MayAcquireConnection(nullptr));
  utils::datetime::MockSleep(kAcquireInterval);
  EXPECT_EQ(kOk, limiter.MayAcquireConnection(nullptr));

  EXPECT_NE(kOk, limiter.MayAcquireConnection(""));
  utils::datetime::MockSleep(kAcquireInterval);
  EXPECT_EQ(kOk, limiter.MayAcquireConnection(""));

  EXPECT_NE(kOk, limiter.MayAcquireConnection("no-schema"));
  utils::datetime::MockSleep(kAcquireInterval);
  EXPECT_EQ(kOk, limiter.MayAcquireConnection("no-schema"));

  EXPECT_NE(kOk, limiter.MayAcquireConnection("http://"));
  utils::datetime::MockSleep(kAcquireInterval);
  EXPECT_EQ(kOk, limiter.MayAcquireConnection("http://"));

  EXPECT_NE(kOk, limiter.MayAcquireConnection("http://test"));
  EXPECT_EQ(kOk, limiter.MayAcquireConnection("https://test"));
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
          while (limiter.MayAcquireConnection(urls[worker_idx].c_str()) ==
                 kOk) {
            ++acquired_ours;
            if (limiter.MayAcquireConnection(urls[worker_idx + 1].c_str()) ==
                kOk) {
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
