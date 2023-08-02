#include <userver/utest/utest.hpp>

#include <atomic>
#include <future>

#include <compiler/relax_cpu.hpp>
#include <logging/tp_logger.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class NonThreadSafeSink final : public logging::impl::BaseSink {
 public:
  void CheckOperationsWerePerformed() {
    EXPECT_GE(flush_count_, 2);
    EXPECT_GE(reopen_count_, 2);
    EXPECT_GE(write_count_, 2);
  }

 protected:
  void Flush() override { Use(flush_count_); }

  void Reopen(logging::impl::ReopenMode) override { Use(reopen_count_); }

  void Write(std::string_view) override { Use(write_count_); }

 private:
  void Use(std::atomic<std::uint64_t>& counter) {
    EXPECT_FALSE(is_used_.exchange(true));
    ++counter;
    EXPECT_TRUE(is_used_.exchange(false));
  }

  std::atomic<bool> is_used_{false};
  std::atomic<std::uint64_t> flush_count_{0};
  std::atomic<std::uint64_t> reopen_count_{0};
  std::atomic<std::uint64_t> write_count_{0};
};

class Backoff final {
 public:
  Backoff() = default;

  void operator()() noexcept {
    for (std::uint64_t i = 0; i < count_; ++i) {
      relax_();
    }
    ++count_;
  }

 private:
  compiler::RelaxCpu relax_;
  std::uint64_t count_{1};
};

}  // namespace

UTEST_MT(TpLogger, SerializesSinkOperations, 4) {
  logging::impl::TpLogger logger{logging::Format::kTskv, "test-logger"};
  logger.AddSink(std::make_unique<NonThreadSafeSink>());
  std::atomic<bool> keep_running{true};

  auto writer_tasks = utils::GenerateFixedArray(2, [&](std::size_t) {
    return std::async([&] {
      Backoff backoff;
      while (keep_running) {
        logger.Log(logging::Level::kInfo, "foo");
        backoff();
      }
    });
  });

  auto flush_tasks = utils::GenerateFixedArray(2, [&](std::size_t) {
    return std::async([&] {
      Backoff backoff;
      while (keep_running) {
        logger.Flush();
        backoff();
      }
    });
  });

  auto reopen_tasks = utils::GenerateFixedArray(2, [&](std::size_t) {
    return engine::AsyncNoSpan([&] {
      Backoff backoff;
      while (keep_running) {
        logger.Reopen(logging::impl::ReopenMode::kAppend);
        backoff();
      }
    });
  });

  engine::SleepFor(std::chrono::milliseconds{100});
  logger.StartConsumerTask(engine::current_task::GetTaskProcessor(), 1 << 8,
                           logging::QueueOverflowBehavior::kDiscard);

  engine::SleepFor(std::chrono::milliseconds{100});
  logger.StopConsumerTask();

  engine::SleepFor(std::chrono::milliseconds{100});
  keep_running = false;

  for (auto& task : writer_tasks) task.get();
  for (auto& task : flush_tasks) task.get();
  for (auto& task : reopen_tasks) task.Get();

  dynamic_cast<NonThreadSafeSink&>(*logger.GetSinks().at(0))
      .CheckOperationsWerePerformed();

  // The test succeeds if
  // - there was no race in the NonThreadSafeSink, and
  // - some operations were successfully performed on the NonThreadSafeSink.
}

USERVER_NAMESPACE_END
