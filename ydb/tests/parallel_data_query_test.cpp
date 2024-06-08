#include <string>

#include <userver/engine/get_all.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/enumerate.hpp>

#include "small_table.hpp"
#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

class YdbExecuteParallelDataQuery : public YdbSmallTableTest {};

void WaitSessionCountIsZero(const NYdb::NTable::TTableClient& table_client_) {
  constexpr auto sleep = std::chrono::milliseconds{10};
  const auto deadline{engine::Deadline::FromDuration(utest::kMaxTestWaitTime)};
  while (table_client_.GetActiveSessionCount() != 0) {
    engine::SleepFor(sleep);
    if (deadline.IsReached()) {
      throw std::runtime_error{"Session count is not zero"};
    }
  }
}

}  // namespace

UTEST_F(YdbExecuteParallelDataQuery, SimpleReadParallel) {
  CreateTable("simple_read", true);
  constexpr int requests_count = 5;

  std::vector<engine::TaskWithResult<ydb::ExecuteResponse>> responses;
  responses.reserve(requests_count);

  for (auto i = 0; i < requests_count; ++i) {
    responses.emplace_back(engine::AsyncNoSpan([&]() {
      return GetTableClient().ExecuteDataQuery(ydb::Query{R"(
            SELECT key, value_str
            FROM simple_read
            WHERE key = "key1";
        )"});
    }));
  }

  auto results = engine::GetAll(responses);
  for (auto& result : results) {
    auto cursor = result.GetSingleCursor();
    AssertArePreFilledRows(std::move(cursor), {1});
  }
}

UTEST_F(YdbExecuteParallelDataQuery, PreparedReadParallel) {
  CreateTable("prepared_read", true);
  constexpr int requests_count = 5;

  const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT key, value_str
        FROM prepared_read
        WHERE key = $search_key;
    )"};

  std::vector<engine::TaskWithResult<ydb::ExecuteResponse>> responses;
  responses.reserve(requests_count);

  for (auto i = 0; i < requests_count; ++i) {
    auto builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key1"}));

    responses.emplace_back(engine::AsyncNoSpan(
        [builder = std::move(builder), query = query, this]() mutable {
          return GetTableClient().ExecuteDataQuery(ydb::OperationSettings{},
                                                   query, std::move(builder));
        }));
  }

  auto results = engine::GetAll(responses);
  for (auto& result : results) {
    AssertArePreFilledRows(result.GetSingleCursor(), {1});
  }
}

UTEST_F(YdbExecuteParallelDataQuery, SessionLeak) {
  CreateTable("session_leak", false);

  constexpr size_t query_count = 100;
  const auto& table_client = GetTableClient().GetNativeTableClient();
  // There may be a TSession after async operation, so we will have to wait for
  // the TSession to be released
  UASSERT_NO_THROW(WaitSessionCountIsZero(table_client));
  std::vector<ydb::ExecuteResponse> results;
  results.reserve(query_count);
  for (std::size_t i = 0; i < query_count; ++i) {
    results.push_back(GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        UPSERT INTO session_leak (key, value_str, value_int)
        VALUES ("key1", "value1", 1), ("key2", "value2", 2);
    )"}));
    UASSERT_NO_THROW(WaitSessionCountIsZero(table_client));
  }
  ASSERT_EQ(results.size(), query_count);
  UASSERT_NO_THROW(WaitSessionCountIsZero(table_client));
}

USERVER_NAMESPACE_END
