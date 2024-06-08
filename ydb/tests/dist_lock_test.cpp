#include <userver/utest/utest.hpp>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/ydb/dist_lock/worker.hpp>

#include <ydb/impl/dist_lock/semaphore_settings.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::chrono::milliseconds kInactiveCheckTime{200};

constexpr std::string_view kCoordinationNode = "test_dist_locks";
constexpr std::string_view kSemaphoreName = "test-semaphore";

class YdbDistLockFixture : public ydb::ClientFixtureBase {
 protected:
  void SetUp() override { CreateCoordinationNode(kCoordinationNode); }

  void TearDown() override { DropCoordinationNode(kCoordinationNode); }

  void CreateCoordinationNode(std::string_view coordination_node) {
    UASSERT_NO_THROW(GetCoordinationClient().CreateNode(coordination_node, {}));
  }

  void DropCoordinationNode(std::string_view coordination_node) {
    UASSERT_NO_THROW(GetCoordinationClient().DropNode(coordination_node));
  }

  auto StartSession(std::string_view coordination_node) {
    return GetCoordinationClient().StartSession(coordination_node, {});
  }

  auto CreateDistLockedWorker(ydb::DistLockedWorker::Callback callback) {
    return ydb::DistLockedWorker{
        engine::current_task::GetTaskProcessor(),
        GetCoordinationClientPtr(),
        std::string{kCoordinationNode},
        std::string{kSemaphoreName},
        ydb::DistLockSettings{},
        std::move(callback),
    };
  }
};

}  // namespace

UTEST_F(YdbDistLockFixture, Work) {
  auto session = StartSession(kCoordinationNode);
  session.CreateSemaphore(kSemaphoreName,
                          ydb::impl::dist_lock::kSemaphoreLimit);

  engine::SingleConsumerEvent event;

  ydb::DistLockedWorker worker = CreateDistLockedWorker([&event, &worker] {
    EXPECT_TRUE(worker.OwnsLock());
    event.Send();
  });

  EXPECT_FALSE(worker.OwnsLock());

  UASSERT_NO_THROW(worker.Start());

  ASSERT_TRUE(event.WaitForEventFor(utest::kMaxTestWaitTime));

  UASSERT_NO_THROW(worker.Stop());

  EXPECT_FALSE(worker.OwnsLock());

  session.DeleteSemaphore(kSemaphoreName);
}

UTEST_F(YdbDistLockFixture, WorkUntilCancellation) {
  auto session = StartSession(kCoordinationNode);
  session.CreateSemaphore(kSemaphoreName,
                          ydb::impl::dist_lock::kSemaphoreLimit);

  engine::SingleConsumerEvent event;

  ydb::DistLockedWorker worker = CreateDistLockedWorker([&event, &worker] {
    EXPECT_TRUE(worker.OwnsLock());
    event.Send();

    while (!engine::current_task::ShouldCancel()) {
      // Imitating a typical while-not-should-cancel loop in DoWork.
      engine::InterruptibleSleepFor(std::chrono::milliseconds{10});
    }
  });

  EXPECT_FALSE(worker.OwnsLock());

  UASSERT_NO_THROW(worker.Start());
  ASSERT_TRUE(event.WaitForEventFor(utest::kMaxTestWaitTime));

  EXPECT_TRUE(worker.OwnsLock());
  engine::SleepFor(std::chrono::milliseconds{100});
  EXPECT_TRUE(worker.OwnsLock());

  UASSERT_NO_THROW(worker.Stop());
  EXPECT_FALSE(worker.OwnsLock());

  session.DeleteSemaphore(kSemaphoreName);
}

UTEST_F(YdbDistLockFixture, Lock) {
  auto session = StartSession(kCoordinationNode);
  session.CreateSemaphore(kSemaphoreName,
                          ydb::impl::dist_lock::kSemaphoreLimit);

  session.AcquireSemaphore(
      kSemaphoreName, NYdb::NCoordination::TAcquireSemaphoreSettings{}.Count(
                          ydb::impl::dist_lock::kSemaphoreLimit));

  engine::SingleConsumerEvent event;

  auto worker = CreateDistLockedWorker([&event] { event.Send(); });

  UASSERT_NO_THROW(worker.Start());

  ASSERT_FALSE(event.WaitForEventFor(kInactiveCheckTime));

  session.ReleaseSemaphore(kSemaphoreName);

  ASSERT_TRUE(event.WaitForEventFor(utest::kMaxTestWaitTime));

  UASSERT_NO_THROW(worker.Stop());

  session.DeleteSemaphore(kSemaphoreName);
}

UTEST_F(YdbDistLockFixture, SessionDestroy) {
  std::optional<ydb::CoordinationSession> session =
      StartSession(kCoordinationNode);
  session->CreateSemaphore(kSemaphoreName,
                           ydb::impl::dist_lock::kSemaphoreLimit);

  session->AcquireSemaphore(
      kSemaphoreName, NYdb::NCoordination::TAcquireSemaphoreSettings{}.Count(
                          ydb::impl::dist_lock::kSemaphoreLimit));

  engine::SingleConsumerEvent event;

  auto worker = CreateDistLockedWorker([&event] { event.Send(); });

  UASSERT_NO_THROW(worker.Start());

  ASSERT_FALSE(event.WaitForEventFor(kInactiveCheckTime));

  /// destroy session
  session.reset();

  ASSERT_TRUE(event.WaitForEventFor(utest::kMaxTestWaitTime));

  UASSERT_NO_THROW(worker.Stop());

  StartSession(kCoordinationNode).DeleteSemaphore(kSemaphoreName);
}

USERVER_NAMESPACE_END
