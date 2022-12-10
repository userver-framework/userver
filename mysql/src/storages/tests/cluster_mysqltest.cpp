#include <userver/utest/utest.hpp>

#include <iostream>

#include <userver/engine/sleep.hpp>

#include <userver/storages/mysql/cluster.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kMasterHost = storages::mysql::ClusterHostType::kMaster;

struct Row final {
  int id{};
  std::string value;
};

struct Id final {
  std::int32_t id{};
};

std::chrono::system_clock::time_point ToMariaDBPrecision(
    std::chrono::system_clock::time_point tp) {
  return std::chrono::time_point_cast<std::chrono::microseconds>(tp);
}

}  // namespace

UTEST(Cluster, TypedWorks) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  auto rows =
      cluster
          .Execute(storages::mysql::ClusterHostType::kMaster, deadline,
                   "SELECT Id, Value FROM test WHERE Id=? OR Value=?", 1, "two")
          .AsVector<Row>();

  for (const auto& [id, value] : rows) {
    // std::cout << id << ": " << value << std::endl;
  }
}

UTEST(Cluster, TypedSizeMismatch) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  auto rows = cluster.Execute(kMasterHost, deadline, "SELECT Id FROM test")
                  .AsVector<Id>();

  // for (const auto [id] : rows) {
  // std::cout << id << " ";
  //}
  std::cout << std::endl;
}

UTEST(Cluster, TypedEmptyResult) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  auto rows =
      cluster.Execute(kMasterHost, deadline,
                      "INSERT INTO test(Id, Value) VALUES(?, ?)", 5, "five");
}

UTEST(Cluster, AsSingleRow) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  const auto select_with_limit = [&cluster, deadline](int limit) {
    return cluster.Execute(kMasterHost, deadline,
                           "SELECT Id, Value FROM test limit ?", limit);
  };

  EXPECT_ANY_THROW(select_with_limit(0).AsSingleRow<Row>());
  EXPECT_ANY_THROW(select_with_limit(2).AsSingleRow<Row>());
  EXPECT_NO_THROW(select_with_limit(1).AsSingleRow<Row>());
}

UTEST(Cluster, InsertMany) {
  const auto deadline =
      engine::Deadline::FromDuration(std::chrono::seconds{500});

  storages::mysql::Cluster cluster{};

  std::vector<Row> rows{{11, "55zxc"}, {22, "66asdwe"}, {33, "77ok"}};
  cluster.InsertMany(deadline, "INSERT INTO test(Id, Value) VALUES(?, ?)",
                     rows);
}

UTEST(Cluster, InsertManySimple) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  std::vector<Id> ids{{1}, {2}, {3}, {4}, {5}, {6}};
  cluster.InsertMany(deadline, "INSERT INTO test(Id) VALUES(?)", ids);
}

UTEST(Cluster, InsertOne) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  Row row{7, "seven"};
  cluster.InsertOne(deadline, "INSERT INTO test(Id, Value) VALUES(?, ?)", row);
}

UTEST(StreamedResult, Works) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  auto stream =
      cluster.Select(kMasterHost, deadline, "SELECT Id, Value FROM test")
          .AsStreamOf<Row>();

  std::move(stream).ForEach(
      [](Row&&) {
        // std::cout << row.id << " " << row.value << std::endl;
      },
      1, deadline);
}

UTEST(Cluster, DISABLED_BigInsert) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  const std::string long_string_to_avoid_sso{
      "hi i am some long string that doesn't fit in sso"};

  constexpr int kRowsCount = 100000;

  std::vector<Row> rows;
  rows.reserve(kRowsCount);
  for (int i = 0; i < kRowsCount; ++i) {
    rows.push_back({i, long_string_to_avoid_sso});
  }

  cluster.InsertMany(deadline, "INSERT INTO test(Id, Value) VALUES(?, ?)",
                     rows);

  rows = cluster.Execute(kMasterHost, deadline, "SELECT Id, Value FROM test")
             .AsVector<Row>();

  // cluster.Execute(kMasterHost, deadline, "DELETE FROM test");
}

UTEST(Cluster, WorksWithConsts) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  const int id = 5;
  cluster.Select(kMasterHost, deadline, "SELECT Id, Value FROM test WHERE Id=?",
                 id);
}

struct AllSupportedTypes final {
  std::uint8_t uint8{};
  std::int8_t int8{};
  std::uint16_t uint16{};
  std::int16_t int16{};
  std::uint32_t uint32{};
  std::int32_t int32{};
  std::uint64_t uint64{};
  std::int64_t int64{};
  float float_t{};
  double double_t{};
  std::string str;
  std::chrono::system_clock::time_point datetime_6_t{};
};

struct We final {
  std::string we;
};

UTEST(Cluster, AllTypesTest) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  const AllSupportedTypes row_to_insert{
      1, 2, 3,    4,    5,        6,
      7, 8, 9.1f, 10.2, "string", std::chrono::system_clock::now()};
  const std::string insert_string{R"(
INSERT INTO all_supported_types_table(
  uint8_t,
  int8_t,
  uint16_t,
  int16_t,
  uint32_t,
  int32_t,
  uint64_t,
  int64_t,
  float_t,
  double_t,
  string_t,
  datetime_6_t
) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))"};
  cluster.InsertOne(deadline, insert_string, row_to_insert);

  const auto row = cluster
                       .Select(kMasterHost, deadline, R"(
SELECT
  uint8_t,
  int8_t,
  uint16_t,
  int16_t,
  uint32_t,
  int32_t,
  uint64_t,
  int64_t,
  float_t,
  double_t,
  string_t,
  datetime_6_t
FROM all_supported_types_table
)")
                       .AsSingleRow<AllSupportedTypes>();

  EXPECT_EQ(row_to_insert.uint8, row.uint8);
  EXPECT_EQ(row_to_insert.int8, row.int8);
  EXPECT_EQ(row_to_insert.uint16, row.uint16);
  EXPECT_EQ(row_to_insert.int16, row.int16);
  EXPECT_EQ(row_to_insert.uint32, row.uint32);
  EXPECT_EQ(row_to_insert.int32, row.int32);
  EXPECT_EQ(row_to_insert.uint64, row.uint64);
  EXPECT_EQ(row_to_insert.int64, row.int64);
  EXPECT_FLOAT_EQ(row_to_insert.float_t, row.float_t);
  EXPECT_DOUBLE_EQ(row_to_insert.double_t, row.double_t);
  EXPECT_EQ(row_to_insert.str, row.str);
  EXPECT_EQ(ToMariaDBPrecision(row_to_insert.datetime_6_t), row.datetime_6_t);

  cluster.Execute(kMasterHost, deadline,
                  "DELETE FROM all_supported_types_table");
}

USERVER_NAMESPACE_END
