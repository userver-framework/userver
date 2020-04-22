#include <utest/utest.hpp>

#include <engine/task/cancel.hpp>
#include <engine/task/task_context.hpp>

#include <storages/redis/impl/exception.hpp>
#include <storages/redis/impl/sentinel.hpp>

class DummySentinel {
 public:
  DummySentinel() : thread_pools_(std::make_shared<redis::ThreadPools>(2, 2)) {
    secdist::RedisSettings settings;
    settings.shards = {"s1"};
    settings.sentinels.emplace_back("localhost", 0);

    sentinel_ = redis::Sentinel::CreateSentinel(thread_pools_, settings,
                                                "group", "client", {""}, {});
    EXPECT_NE(sentinel_.get(), nullptr);
  }

  std::shared_ptr<redis::Sentinel> GetSentinel() { return sentinel_; }

 private:
  std::shared_ptr<redis::ThreadPools> thread_pools_;
  std::shared_ptr<redis::Sentinel> sentinel_;
};

TEST(Sentinel, CancelRequestPre) {
  RunInCoro([] {
    DummySentinel sentinel;

    engine::current_task::GetCurrentTaskContext()->RequestCancel(
        engine::Task::CancellationReason::kUserRequest);
    EXPECT_THROW(sentinel.GetSentinel()->Get("123"),
                 redis::RequestCancelledException);
  });
}

TEST(Sentinel, CancelRequestWithShardParamPre) {
  RunInCoro([] {
    DummySentinel sentinel;

    engine::current_task::GetCurrentTaskContext()->RequestCancel(
        engine::Task::CancellationReason::kUserRequest);
    EXPECT_THROW(sentinel.GetSentinel()->Keys("123", 0),
                 redis::RequestCancelledException);
  });
}

TEST(Sentinel, CancelRequestPost) {
  RunInCoro([] {
    DummySentinel sentinel;

    auto request = sentinel.GetSentinel()->Get("123");
    engine::current_task::GetCurrentTaskContext()->RequestCancel(
        engine::Task::CancellationReason::kUserRequest);
    EXPECT_THROW(request.Get(), redis::RequestCancelledException);
  });
}
