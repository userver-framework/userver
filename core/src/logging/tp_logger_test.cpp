#include <logging/tp_logger.hpp>

#include <gmock/gmock.h>
#include <boost/algorithm/string.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/pretty_format.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

#include <logging/logging_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using QueueOverflowBehavior = logging::LoggerConfig::QueueOverflowBehavior;

constexpr std::size_t kLoggingTestIterations = 400;
constexpr std::size_t kLoggingRecursionDepth = 5;

std::string_view LogRecursiveHelper(
    std::shared_ptr<logging::impl::TpLogger> logger, std::size_t from,
    std::size_t to) {
  if (from < to) {
    LOG_INFO_TO(logger) << from << LogRecursiveHelper(logger, from + 1, to);
  }
  return " recursion";
}

class LoggingTestCoro : public LoggingTestBase {
 public:
  enum LogTestMTMode {
    kTestLogging = 0,
    kTestLogFlush = 1,
    kTestLogSync = 2,
    kTestLogFlushSync = kTestLogFlush | kTestLogSync,
    kTestLogCancel = 4,
    kTestLogFlushCancel = kTestLogCancel | kTestLogFlush,
    kTestLogSyncCancel = kTestLogCancel | kTestLogSync,
    kTestLogFlushSyncCancel = kTestLogCancel | kTestLogFlush | kTestLogSync,
    kTestLogStdThread = 8,
    kTestLogStdThreadFlush = kTestLogStdThread | kTestLogFlush,
    kTestLogStdThreadFlushSyncCancel =
        kTestLogStdThread | kTestLogFlushSyncCancel,
  };

  std::shared_ptr<logging::impl::TpLogger> StartAsyncLogger(
      std::size_t queue_size_max = 10,
      QueueOverflowBehavior on_overflow = QueueOverflowBehavior::kDiscard) {
    UASSERT_MSG(engine::current_task::IsTaskProcessorThread(),
                "Misconfigured test. Should be run in coroutine environment");

    auto logger = GetStreamLogger();

    stats_holder_ = stats_storage_.RegisterWriter(
        "logger", [&logger](utils::statistics::Writer& writer) {
          writer = logger->GetStatistics();
        });

    logger->StartAsync(engine::current_task::GetTaskProcessor(), queue_size_max,
                       on_overflow);

    // Tracing should not break the TpLogger
    logger->SetLevel(logging::Level::kTrace);

    return logger;
  }

  int64_t GetMetric(std::string metric = {},
                    utils::statistics::Label label = {}) {
    std::vector<utils::statistics::Label> labels{};
    if (!label.Name().empty()) {
      labels.push_back(label);
    }
    const auto snapshot =
        utils::statistics::Snapshot(stats_storage_, "logger", labels);
    return snapshot.SingleMetric(metric).AsInt();
  }

  void LogTestMT(std::shared_ptr<logging::impl::TpLogger> logger,
                 std::size_t thread_count, LogTestMTMode mode) const {
    std::vector<engine::TaskWithResult<void>> tasks;

    if (!(mode & kTestLogFlush)) {
      // Leave one thread for processing the logs
      --thread_count;
    }

    tasks.reserve(thread_count);

    std::thread std_thread;

    for (std::size_t thread_index = 0; thread_index < thread_count;
         ++thread_index) {
      const auto lambda = [thread_index, &logger, mode]() {
        for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
          LOG_INFO_TO(logger) << i << " at " << thread_index;

          if (mode & kTestLogFlush) {
            logger->Flush();
          }

          if (thread_index != 0) {
            if (mode & kTestLogCancel && i == kLoggingTestIterations / 4 * 3) {
              engine::current_task::GetCancellationToken().RequestCancel();
            }
            if (mode & kTestLogSync && i == kLoggingTestIterations / 4 * 3) {
              logger->SwitchToSyncMode();
            }
          }
        }
      };

      if (thread_index == 0 && (mode & kTestLogStdThread)) {
        std_thread = std::thread{lambda};
      } else {
        tasks.push_back(engine::AsyncNoSpan(lambda));
      }
    }

    for (auto& t : tasks) {
      t.Get();
    }

    if (std_thread.joinable()) {
      std_thread.join();
    }

    logger->SwitchToSyncMode();

    auto logs = GetStreamString();
    for (std::size_t thread_index = 0; thread_index < thread_count;
         ++thread_index) {
      for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
        EXPECT_THAT(logs, testing::HasSubstr(
                              fmt::format("text={} at {}", i, thread_index)));
      }
    }
  }

 protected:
  LoggingTestCoro() : LoggingTestBase(logging::Format::kTskv) {
    SetDefaultLoggerLevel(logging::Level::kError);
  }

  ~LoggingTestCoro() override { stats_holder_.Unregister(); }

 private:
  utils::statistics::Storage stats_storage_{};
  utils::statistics::Entry stats_holder_;
};

}  // namespace

TEST_F(LoggingTest, TpLoggerNoop) {
  ASSERT_FALSE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should not be run in coroutine environment";

  GetStreamLogger();
  EXPECT_EQ(GetRecordsCount(), 0);
}

TEST_F(LoggingTest, TpLoggerBasic) {
  ASSERT_FALSE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should not be run in coroutine environment";

  auto logger = GetStreamLogger();

  LOG_INFO_TO(logger) << "Some log";
  logger->Flush();
  EXPECT_THAT(LoggedText(), testing::HasSubstr("Some log"));

  logger->SetLevel(logging::Level::kError);
  ASSERT_EQ(logger->GetLevel(), logging::Level::kError);

  LOG_DEBUG_TO(logger) << "Some log 2";
  EXPECT_EQ(GetRecordsCount(), 1);
  LOG_INFO_TO(logger) << "Some log 3";
  EXPECT_EQ(GetRecordsCount(), 1);
}

TEST_F(LoggingTest, TpLoggerBasicRecursive) {
  ASSERT_FALSE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should not be run in coroutine environment";

  auto logger = GetStreamLogger();
  LogRecursiveHelper(logger, 0, kLoggingRecursionDepth);

  const auto logs = GetStreamString();
  for (std::size_t i = 0; i < kLoggingRecursionDepth; ++i) {
    EXPECT_THAT(logs, testing::HasSubstr(fmt::format("text={} recursion", i)));
  }
  EXPECT_EQ(GetRecordsCount(), kLoggingRecursionDepth);
}

TEST_F(LoggingTest, TpLoggerBasicMT) {
  ASSERT_FALSE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should not be run in coroutine environment";

  auto logger = GetStreamLogger();

  std::thread task([&logger]() {
    for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
      LOG_INFO_TO(logger) << i << " at 1";
    }
  });

  ASSERT_TRUE(kLoggingTestIterations % kLoggingRecursionDepth == 0)
      << "Test Misconfigured";
  for (std::size_t i = 0; i < kLoggingTestIterations;
       i += kLoggingRecursionDepth) {
    LogRecursiveHelper(logger, i, i + kLoggingRecursionDepth);
  }

  task.join();

  const auto logs = GetStreamString();
  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    EXPECT_THAT(logs, testing::HasSubstr(fmt::format("text={} at 1", i)));
    EXPECT_THAT(logs, testing::HasSubstr(fmt::format("text={} recursion", i)));
  }
  EXPECT_EQ(GetRecordsCount(), kLoggingTestIterations * 2);
}

TEST_F(LoggingTest, TpLoggerBasicFlushMT) {
  ASSERT_FALSE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should not be run in coroutine environment";

  auto logger = GetStreamLogger();

  std::thread task([&logger]() {
    for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
      LOG_INFO_TO(logger) << i << " at 1";
    }
  });

  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    LOG_INFO_TO(logger) << i << " at 0";
    logger->Flush();
  }

  task.join();

  const auto logs = GetStreamString();
  for (std::size_t thread_index = 0; thread_index < 2; ++thread_index) {
    for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
      EXPECT_THAT(logs, testing::HasSubstr(
                            fmt::format("text={} at {}", i, thread_index)));
    }
  }
  EXPECT_EQ(GetRecordsCount(), kLoggingTestIterations * 2);
}

TEST_F(LoggingTest, TpLoggerBasicToSyncNoop) {
  ASSERT_FALSE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should not be run in coroutine environment";
  auto logger = GetStreamLogger();

  logger->SwitchToSyncMode();
}

TEST_F(LoggingTest, TpLoggerBasicToSyncNoopTwice) {
  ASSERT_FALSE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should not be run in coroutine environment";
  auto logger = GetStreamLogger();

  logger->SwitchToSyncMode();
  logger->SwitchToSyncMode();
}

UTEST_F(LoggingTestCoro, TpLoggerNoop) {
  ASSERT_TRUE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should be run in coroutine environment";
  GetStreamLogger();
  EXPECT_EQ(GetRecordsCount(), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasic) {
  ASSERT_TRUE(engine::current_task::IsTaskProcessorThread())
      << "Misconfigured test. Should be run in coroutine environment";
  auto logger = GetStreamLogger();

  LOG_INFO_TO(logger) << "Some log";
  logger->Flush();
  EXPECT_THAT(LoggedText(), testing::HasSubstr("Some log"));

  logger->SetLevel(logging::Level::kError);
  ASSERT_EQ(logger->GetLevel(), logging::Level::kError);

  LOG_DEBUG_TO(logger) << "Some log 2";
  EXPECT_EQ(GetRecordsCount(), 1);
  LOG_INFO_TO(logger) << "Some log 3";
  EXPECT_EQ(GetRecordsCount(), 1);
}

UTEST_F(LoggingTestCoro, TpLoggerNoopAsyncToSync) {
  auto logger = StartAsyncLogger();
  logger->SwitchToSyncMode();
}

UTEST_F(LoggingTestCoro, TpLoggerNoopAsyncToSyncTwice) {
  auto logger = StartAsyncLogger();
  logger->SwitchToSyncMode();
  logger->SwitchToSyncMode();
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsync) {
  auto logger = StartAsyncLogger();

  LOG_INFO_TO(logger) << "Some log";
  logger->Flush();
  EXPECT_THAT(LoggedText(), testing::HasSubstr("Some log"));
  logger->SwitchToSyncMode();
  EXPECT_EQ(GetRecordsCount(), 1);

  EXPECT_EQ(GetMetric("total"), 1);
  EXPECT_EQ(GetMetric("dropped"), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 1);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncMetrics) {
  auto logger = StartAsyncLogger();

  LOG_INFO_TO(logger) << "Some log";
  LOG_INFO_TO(logger) << "Some log";
  LOG_INFO_TO(logger) << "Some log";
  LOG_INFO_TO(logger) << "Some log";
  LOG_INFO_TO(logger) << "Some log";
  LOG_INFO_TO(logger) << "Some log";
  LOG_INFO_TO(logger) << "Some log";
  LOG_INFO_TO(logger) << "Some log";

  logger->Flush();

  LOG_CRITICAL_TO(logger) << "Critical log";
  LOG_CRITICAL_TO(logger) << "Critical log";
  LOG_CRITICAL_TO(logger) << "Critical log";
  LOG_CRITICAL_TO(logger) << "Critical log";
  LOG_CRITICAL_TO(logger) << "Critical log";
  LOG_CRITICAL_TO(logger) << "Critical log";

  LOG_ERROR_TO(logger) << "Error log";
  LOG_ERROR_TO(logger) << "Error log";
  LOG_ERROR_TO(logger) << "Error log";
  LOG_ERROR_TO(logger) << "Error log";
  LOG_ERROR_TO(logger) << "Error log";

  LOG_DEBUG_TO(logger) << "Debug log";
  LOG_DEBUG_TO(logger) << "Debug log";
  LOG_DEBUG_TO(logger) << "Debug log";
  LOG_DEBUG_TO(logger) << "Debug log";

  LOG_WARNING_TO(logger) << "Warning log";
  LOG_WARNING_TO(logger) << "Warning log";
  LOG_WARNING_TO(logger) << "Warning log";

  LOG_TRACE_TO(logger) << "Trace log";
  LOG_TRACE_TO(logger) << "Trace log";

  logger->Flush();
  logger->SwitchToSyncMode();
  EXPECT_EQ(GetRecordsCount(), 18);
  EXPECT_EQ(GetMetric("total"), 28);
  EXPECT_EQ(GetMetric("dropped"), 10);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 8);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 4);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 3);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 6);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 5);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 2);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncToSyncBeforeLog) {
  auto logger = StartAsyncLogger();

  logger->SwitchToSyncMode();
  LOG_INFO_TO(logger) << "Some log";
  logger->Flush();
  EXPECT_THAT(LoggedText(), testing::HasSubstr("Some log"));

  EXPECT_EQ(GetMetric("total"), 1);
  EXPECT_EQ(GetMetric("dropped"), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 1);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncToSyncBeforeFlush) {
  auto logger = StartAsyncLogger();

  LOG_INFO_TO(logger) << "Some log";
  logger->SwitchToSyncMode();
  logger->Flush();
  EXPECT_THAT(LoggedText(), testing::HasSubstr("Some log"));

  EXPECT_EQ(GetMetric("total"), 1);
  EXPECT_EQ(GetMetric("dropped"), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 1);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncToSyncAfterFlush) {
  auto logger = StartAsyncLogger();

  LOG_INFO_TO(logger) << "Some log";
  logger->Flush();
  logger->SwitchToSyncMode();
  EXPECT_THAT(LoggedText(), testing::HasSubstr("Some log"));

  EXPECT_EQ(GetMetric("total"), 1);
  EXPECT_EQ(GetMetric("dropped"), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 1);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncToSyncFlushOverflow) {
  auto logger = StartAsyncLogger(2);

  LOG_INFO_TO(logger) << "Some log";
  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    // Smoke test that Flush is not lost and not overflowed
    logger->Flush();
  }
  logger->SwitchToSyncMode();
  EXPECT_THAT(LoggedText(), testing::HasSubstr("Some log"));

  EXPECT_EQ(GetMetric("total"), 1);
  EXPECT_EQ(GetMetric("dropped"), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 1);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicSyncFlushOverflow) {
  auto logger = StartAsyncLogger(2);

  LOG_INFO_TO(logger) << "Some log";
  logger->SwitchToSyncMode();
  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    // Smoke test that Flush is not lost and not overflowed
    logger->Flush();
  }
  EXPECT_THAT(LoggedText(), testing::HasSubstr("Some log"));

  EXPECT_EQ(GetMetric("total"), 1);
  EXPECT_EQ(GetMetric("dropped"), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 1);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncOverflow) {
  auto logger = StartAsyncLogger(2);

  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    LOG_INFO_TO(logger) << i;
  }
  logger->SwitchToSyncMode();

  EXPECT_GE(GetRecordsCount(), 1) << "Nothing was logged";
  EXPECT_LT(GetRecordsCount(), kLoggingTestIterations) << "Nothing was skipped";

  EXPECT_EQ(GetMetric("total"), 400);
  EXPECT_EQ(GetMetric("dropped"), 398);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 400);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncOverflowCancelled) {
  auto logger = StartAsyncLogger(2);

  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    LOG_INFO_TO(logger) << i;

    if (i == kLoggingTestIterations / 2) {
      engine::current_task::GetCancellationToken().RequestCancel();
    }
  }
  logger->SwitchToSyncMode();

  EXPECT_GE(GetRecordsCount(), 1) << "Nothing was logged";
  EXPECT_LT(GetRecordsCount(), kLoggingTestIterations) << "Nothing was skipped";

  EXPECT_EQ(GetMetric("total"), 400);
  EXPECT_EQ(GetMetric("dropped"), 398);
  EXPECT_EQ(GetMetric("by_level", {"level", "info"}), 400);
  EXPECT_EQ(GetMetric("by_level", {"level", "debug"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "warning"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "critical"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "error"}), 0);
  EXPECT_EQ(GetMetric("by_level", {"level", "trace"}), 0);
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncOverflowFlushCancelled) {
  auto logger = StartAsyncLogger(2);

  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    LOG_INFO_TO(logger) << i;

    if (i == kLoggingTestIterations / 2) {
      engine::current_task::GetCancellationToken().RequestCancel();
    }

    if (i % 10 == 0) {
      logger->Flush();
    }
  }
  logger->SwitchToSyncMode();

  EXPECT_GE(GetRecordsCount(), 1) << "Nothing was logged";
  EXPECT_LT(GetRecordsCount(), kLoggingTestIterations) << "Nothing was skipped";
}

UTEST_F(LoggingTestCoro, TpLoggerBasicAsyncOverflowBlocking) {
  auto logger = StartAsyncLogger(2, QueueOverflowBehavior::kBlock);

  // Tracing should not break the TpLogger
  ASSERT_EQ(logger->GetLevel(), logging::Level::kTrace) << "Misconfigured test";

  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    LOG_INFO_TO(logger) << i;
  }
  logger->SwitchToSyncMode();

  const auto logs = GetStreamString();
  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    EXPECT_THAT(logs, testing::HasSubstr(fmt::format("text={}", i)));
  }

  EXPECT_EQ(GetRecordsCount(), kLoggingTestIterations);
}

UTEST_F(LoggingTestCoro, TpLoggerFlush) {
  auto logger = StartAsyncLogger(2);

  LOG_INFO_TO(logger) << "1";
  LOG_INFO_TO(logger) << "2";
  logger->Flush();
  LOG_INFO_TO(logger) << "3";
  LOG_TO(logger, logging::Level::kNone) << "oops";
  LOG_INFO_TO(logger) << "4";
  logger->SwitchToSyncMode();

  EXPECT_THAT(GetStreamString(), testing::HasSubstr("text=1"));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("text=2"));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("text=3"));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("text=oops")));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("text=4"));

  EXPECT_EQ(GetRecordsCount(), 4);
}

UTEST_F(LoggingTestCoro, TpLoggerFlushMultiple) {
  constexpr std::size_t kQueueSize = 16;
  auto logger = StartAsyncLogger(kQueueSize);

  // Tracing should not break the TpLogger
  ASSERT_EQ(logger->GetLevel(), logging::Level::kTrace) << "Misconfigured test";

  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    LOG_INFO_TO(logger) << i;
    if (i % kQueueSize == 0) {
      logger->Flush();
    }
  }
  logger->SwitchToSyncMode();

  const auto logs = GetStreamString();
  for (std::size_t i = 0; i < kLoggingTestIterations; ++i) {
    EXPECT_THAT(logs, testing::HasSubstr(fmt::format("text={}", i)));
  }

  EXPECT_EQ(GetRecordsCount(), kLoggingTestIterations);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogging);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleDefaultMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);

  // Make sure that tracing does not break the default TpLogger
  SetDefaultLogger(logger);
  ASSERT_EQ(logger->GetLevel(), logging::Level::kTrace);

  LogTestMT(logger, GetThreadCount(), kTestLogging);
  EXPECT_GE(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleBlockingMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kBlock);
  LogTestMT(logger, GetThreadCount(), kTestLogging);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleFlushMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger = StartAsyncLogger(GetThreadCount() * 2 /* log + flush */,
                                 QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogFlush);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleSyncMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogSync);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleFlushSyncMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogFlushSync);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleCancelMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogCancel);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleFlushCancelMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogFlushCancel);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleFlushSyncCancelMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogFlushSyncCancel);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleDefaultFlushMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);

  // Make sure that tracing does not break the default TpLogger
  SetDefaultLogger(logger);
  ASSERT_EQ(logger->GetLevel(), logging::Level::kTrace);

  LogTestMT(logger, GetThreadCount(), kTestLogFlush);
  EXPECT_GE(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleDefaultFlushSyncMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);

  // Make sure that tracing does not break the default TpLogger
  SetDefaultLogger(logger);
  ASSERT_EQ(logger->GetLevel(), logging::Level::kTrace);

  LogTestMT(logger, GetThreadCount(), kTestLogFlushSync);
  EXPECT_GE(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleBlockingFlushMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger = StartAsyncLogger(GetThreadCount() * 2 /* log + flush */,
                                 QueueOverflowBehavior::kBlock);
  LogTestMT(logger, GetThreadCount(), kTestLogFlush);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleBlockingFlushSyncMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger = StartAsyncLogger(GetThreadCount() * 2 /* log + flush */,
                                 QueueOverflowBehavior::kBlock);
  LogTestMT(logger, GetThreadCount(), kTestLogFlushSync);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleBlockingSyncMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kBlock);
  LogTestMT(logger, GetThreadCount(), kTestLogSync);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleBlockingCancelMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kBlock);
  LogTestMT(logger, GetThreadCount(), kTestLogCancel);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleBlockingSyncCancelMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kBlock);
  LogTestMT(logger, GetThreadCount(), kTestLogSyncCancel);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleBlockingFlushSyncCancelMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger = StartAsyncLogger(GetThreadCount() * 2 /* log + flush */,
                                 QueueOverflowBehavior::kBlock);
  LogTestMT(logger, GetThreadCount(), kTestLogFlushSyncCancel);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleStdThreadMT, 4) {
  const std::size_t message_count =
      kLoggingTestIterations * (GetThreadCount() - 1);
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogStdThread);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleStdThreadFlushMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger = StartAsyncLogger(GetThreadCount() * 2 /* log + flush */,
                                 QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogStdThreadFlush);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

UTEST_F_MT(LoggingTestCoro, TpLoggerLogMultipleStdThreadFlushSyncCancelMT, 4) {
  const std::size_t message_count = kLoggingTestIterations * GetThreadCount();
  auto logger =
      StartAsyncLogger(message_count * 10, QueueOverflowBehavior::kDiscard);
  LogTestMT(logger, GetThreadCount(), kTestLogStdThreadFlushSyncCancel);
  EXPECT_EQ(GetRecordsCount(), message_count);
}

USERVER_NAMESPACE_END
