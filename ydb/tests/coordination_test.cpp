#include <userver/utest/utest.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kCoordinationNode = "test_coordination_node";

constexpr std::string_view kSemaphoreName = "test-semaphore";
constexpr std::uint64_t kSemaphoreLimit = 1;

class YdbCoordinationFixture : public ydb::ClientFixtureBase {
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
};

}  // namespace

UTEST_F(YdbCoordinationFixture, SessionState) {
  auto session = StartSession(kCoordinationNode);
  ASSERT_EQ(NYdb::NCoordination::ESessionState::ATTACHED,
            session.GetSessionState());
  ASSERT_EQ(NYdb::NCoordination::EConnectionState::CONNECTED,
            session.GetConnectionState());
}

UTEST_F(YdbCoordinationFixture, Close) {
  auto session = StartSession(kCoordinationNode);
  UASSERT_NO_THROW(session.Close());
}

UTEST_F(YdbCoordinationFixture, Ping) {
  auto session = StartSession(kCoordinationNode);
  UASSERT_NO_THROW(session.Ping());
}

UTEST_F(YdbCoordinationFixture, Reconnect) {
  auto session = StartSession(kCoordinationNode);
  UASSERT_NO_THROW(session.Reconnect());
}

UTEST_F(YdbCoordinationFixture, CreateSemaphore) {
  auto session = StartSession(kCoordinationNode);

  UASSERT_NO_THROW(session.CreateSemaphore(kSemaphoreName, kSemaphoreLimit));
  NYdb::NCoordination::TSemaphoreDescription desc;
  UASSERT_NO_THROW(
      desc = session.DescribeSemaphore(
          kSemaphoreName,
          NYdb::NCoordination::TDescribeSemaphoreSettings{}.IncludeOwners()));
  EXPECT_EQ(kSemaphoreName, desc.GetName());
  EXPECT_TRUE(desc.GetData().empty());
  EXPECT_EQ(0, desc.GetCount());
  EXPECT_EQ(kSemaphoreLimit, desc.GetLimit());
  EXPECT_TRUE(desc.GetOwners().empty());

  UASSERT_NO_THROW(session.DeleteSemaphore(kSemaphoreName));
  UASSERT_THROW(session.DescribeSemaphore(kSemaphoreName, {}),
                ydb::YdbResponseError);
}

UTEST_F(YdbCoordinationFixture, UpdateSemaphore) {
  auto session = StartSession(kCoordinationNode);
  UASSERT_NO_THROW(session.CreateSemaphore(kSemaphoreName, kSemaphoreLimit));

  constexpr std::string_view semaphore_data = "test_semaphore_data";
  UASSERT_NO_THROW(session.UpdateSemaphore(kSemaphoreName, semaphore_data));
  NYdb::NCoordination::TSemaphoreDescription desc;
  UASSERT_NO_THROW(desc = session.DescribeSemaphore(kSemaphoreName, {}));
  EXPECT_EQ(kSemaphoreName, desc.GetName());
  EXPECT_EQ(semaphore_data, desc.GetData());

  UASSERT_NO_THROW(session.DeleteSemaphore(kSemaphoreName));
}

UTEST_F(YdbCoordinationFixture, AcquireRelease) {
  auto session = StartSession(kCoordinationNode);
  UASSERT_NO_THROW(session.CreateSemaphore(kSemaphoreName, kSemaphoreLimit));

  {
    ASSERT_TRUE(session.AcquireSemaphore(
        kSemaphoreName, NYdb::NCoordination::TAcquireSemaphoreSettings{}.Count(
                            kSemaphoreLimit)));

    NYdb::NCoordination::TSemaphoreDescription desc1;
    UASSERT_NO_THROW(
        desc1 = session.DescribeSemaphore(
            kSemaphoreName,
            NYdb::NCoordination::TDescribeSemaphoreSettings{}.IncludeOwners()));
    ASSERT_EQ(kSemaphoreName, desc1.GetName());
    ASSERT_EQ(kSemaphoreLimit, desc1.GetCount());
    ASSERT_EQ(1, desc1.GetOwners().size());
    ASSERT_EQ(session.GetSessionId(), desc1.GetOwners()[0].GetSessionId());
  }

  {
    ASSERT_TRUE(session.ReleaseSemaphore(kSemaphoreName));

    NYdb::NCoordination::TSemaphoreDescription desc2;
    UASSERT_NO_THROW(
        desc2 = session.DescribeSemaphore(
            kSemaphoreName,
            NYdb::NCoordination::TDescribeSemaphoreSettings{}.IncludeOwners()));
    ASSERT_EQ(kSemaphoreName, desc2.GetName());
    ASSERT_EQ(0, desc2.GetCount());
    ASSERT_TRUE(desc2.GetOwners().empty());
  }

  UASSERT_NO_THROW(session.DeleteSemaphore(kSemaphoreName));
}

UTEST_F(YdbCoordinationFixture, Lock) {
  auto session1 = StartSession(kCoordinationNode);
  UASSERT_NO_THROW(session1.CreateSemaphore(kSemaphoreName, kSemaphoreLimit));

  ASSERT_TRUE(session1.AcquireSemaphore(
      kSemaphoreName,
      NYdb::NCoordination::TAcquireSemaphoreSettings{}.Count(kSemaphoreLimit)));

  /// second acquire on session1 - nothing changed
  ASSERT_TRUE(session1.AcquireSemaphore(
      kSemaphoreName,
      NYdb::NCoordination::TAcquireSemaphoreSettings{}.Count(kSemaphoreLimit)));

  auto task = engine::AsyncNoSpan([this] {
    auto session2 = StartSession(kCoordinationNode);
    ASSERT_TRUE(session2.AcquireSemaphore(
        kSemaphoreName, NYdb::NCoordination::TAcquireSemaphoreSettings{}.Count(
                            kSemaphoreLimit)));
    ASSERT_TRUE(session2.ReleaseSemaphore(kSemaphoreName));
  });

  task.WaitFor(std::chrono::milliseconds(200));
  ASSERT_FALSE(task.IsFinished());

  ASSERT_TRUE(session1.ReleaseSemaphore(kSemaphoreName));

  task.WaitFor(std::chrono::milliseconds(200));
  ASSERT_TRUE(task.IsFinished());

  UASSERT_NO_THROW(session1.DeleteSemaphore(kSemaphoreName));
}

USERVER_NAMESPACE_END
