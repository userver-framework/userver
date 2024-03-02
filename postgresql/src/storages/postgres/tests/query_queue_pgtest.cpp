#include <storages/postgres/tests/util_pgtest.hpp>

#include <userver/storages/postgres/query_queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
namespace pg = storages::postgres;

constexpr pg::TimeoutDuration kPrepareTimeout{utest::kMaxTestWaitTime};
constexpr pg::TimeoutDuration kCollectTimeout{utest::kMaxTestWaitTime};
constexpr pg::CommandControl kDefaultCC{kPrepareTimeout, kPrepareTimeout};

using QueryQueueResult = std::vector<pg::ResultSet>;
}  // namespace

UTEST_P(PostgreConnection, QueryQueueSelectOne) {
  CheckConnection(GetConn());
  if (!GetConn()->IsPipelineActive()) {
    return;
  }

  pg::QueryQueue query_queue{kDefaultCC, std::move(GetConn())};

  UEXPECT_NO_THROW(query_queue.Push(kDefaultCC, "SELECT 1"));
  QueryQueueResult result{};
  UEXPECT_NO_THROW(result = query_queue.Collect(kCollectTimeout));

  ASSERT_EQ(1, result.size());
  EXPECT_EQ(1, result.front().AsSingleRow<int>());
}

UTEST_P(PostgreConnection, QueryQueueSelectMultiple) {
  CheckConnection(GetConn());
  if (!GetConn()->IsPipelineActive()) {
    return;
  }

  pg::QueryQueue query_queue{kDefaultCC, std::move(GetConn())};

  constexpr int kQueriesCount = 5;
  for (int i = 0; i < kQueriesCount; ++i) {
    UEXPECT_NO_THROW(query_queue.Push(kDefaultCC, "SELECT $1", i));
  }
  QueryQueueResult result{};
  UEXPECT_NO_THROW(result = query_queue.Collect(kCollectTimeout));

  ASSERT_EQ(kQueriesCount, result.size());
  for (int i = 0; i < kQueriesCount; ++i) {
    EXPECT_EQ(i, result[i].AsSingleRow<int>());
  }
}

UTEST_P(PostgreConnection, QueryQueueTimeout) {
  CheckConnection(GetConn());
  if (!GetConn()->IsPipelineActive()) {
    return;
  }

  pg::QueryQueue query_queue{kDefaultCC, std::move(GetConn())};

  UEXPECT_NO_THROW(query_queue.Push(kDefaultCC, "SELECT 1"));
  UEXPECT_NO_THROW(query_queue.Push(kDefaultCC, "SELECT pg_sleep(5)"));

  QueryQueueResult result{};
  UEXPECT_THROW(result = query_queue.Collect(std::chrono::milliseconds{100}),
                pg::ConnectionTimeoutError);
}

UTEST_P(PostgreConnection, QueryQueueEmpty) {
  CheckConnection(GetConn());
  if (!GetConn()->IsPipelineActive()) {
    return;
  }

  const pg::QueryQueue query_queue{kDefaultCC, std::move(GetConn())};
  // Yes, that's it: just check the construction-destruction cycle
}

UTEST_P(PostgreConnection, QueryQueueEmptyCollect) {
  CheckConnection(GetConn());
  if (!GetConn()->IsPipelineActive()) {
    return;
  }

  pg::QueryQueue query_queue{kDefaultCC, std::move(GetConn())};

  QueryQueueResult result{};
  UEXPECT_NO_THROW(result = query_queue.Collect(kCollectTimeout));

  EXPECT_TRUE(result.empty());
}

UTEST_P(PostgreConnection, QueryQueueDestroyWithoutCollect) {
  CheckConnection(GetConn());
  if (!GetConn()->IsPipelineActive()) {
    return;
  }

  pg::QueryQueue query_queue{kDefaultCC, std::move(GetConn())};

  query_queue.Push(kDefaultCC, "SELECT 1");
  // Yes, that's it: just check the destruction with pushed queries
}

UTEST_P(PostgreConnection, QueryQueueMoveCtor) {
  CheckConnection(GetConn());
  if (!GetConn()->IsPipelineActive()) {
    return;
  }

  pg::QueryQueue query_queue{kDefaultCC, std::move(GetConn())};
  query_queue.Push(kDefaultCC, "SELECT 1");

  pg::QueryQueue other_queue{std::move(query_queue)};
  QueryQueueResult result{};
  UEXPECT_NO_THROW(result = other_queue.Collect(kCollectTimeout));

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result.front().AsSingleRow<int>(), 1);
}

UTEST_P(PostgreConnection, QueryQueueActuallyFifo) {
  CheckConnection(GetConn());
  if (!GetConn()->IsPipelineActive()) {
    return;
  }

  GetConn()->Execute(
      "CREATE TEMP TABLE qq_fifo_test(id INT PRIMARY KEY, value INT)");

  const pg::Query kUpsertQuery{
      "INSERT INTO qq_fifo_test(id, value) VALUES($1, $2) ON CONFLICT(id) DO "
      "UPDATE SET value = $2"};
  const pg::Query kSelectQuery{"SELECT value FROM qq_fifo_test WHERE ID = $1"};
  constexpr std::size_t kInsertSelectPairsCount = 3;
  constexpr int kRowId = 1;

  pg::QueryQueue query_queue{kDefaultCC, std::move(GetConn())};
  for (std::size_t i = 0; i < kInsertSelectPairsCount; ++i) {
    query_queue.Push(kDefaultCC, kUpsertQuery, kRowId, static_cast<int>(i));
    query_queue.Push(kDefaultCC, kSelectQuery, kRowId);
  }

  QueryQueueResult result{};
  UEXPECT_NO_THROW(result = query_queue.Collect(kCollectTimeout));

  ASSERT_EQ(result.size(), kInsertSelectPairsCount * 2);
  for (std::size_t i = 0; i < kInsertSelectPairsCount; ++i) {
    const auto& insert_res = result[2 * i];
    const auto& select_res = result[2 * i + 1];
    EXPECT_EQ(insert_res.RowsAffected(), 1);
    EXPECT_EQ(select_res.AsSingleRow<int>(), i);
  }
}

USERVER_NAMESPACE_END
