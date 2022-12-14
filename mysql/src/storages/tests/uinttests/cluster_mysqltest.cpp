#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

#include <iostream>

#include <userver/engine/sleep.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

constexpr auto kMasterHost = storages::mysql::ClusterHostType::kMaster;

struct Row final {
  int id{};
  std::string value;

  bool operator==(const Row& other) const {
    return id == other.id && value == other.value;
  }
};

struct Id final {
  std::int32_t id{};
};

}  // namespace

/*UTEST(Cluster, TypedWorks) {
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
}*/

UTEST(StreamedResult, Works) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, "Id INT NOT NULL, Value TEXT NOT NULL"};

  constexpr std::size_t kRowsCount = 100;
  std::vector<Row> rows_to_insert;
  rows_to_insert.reserve(kRowsCount);

  for (std::size_t i = 0; i < kRowsCount; ++i) {
    rows_to_insert.push_back(
        {static_cast<std::int32_t>(i), utils::generators::GenerateUuid()});

    cluster->InsertOne(
        cluster.GetDeadline(),
        table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
        rows_to_insert.back());
  }

  /*cluster->InsertMany(
      cluster.GetDeadline(),
      table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
      rows_to_insert);*/

  std::vector<Row> db_rows;
  db_rows.reserve(kRowsCount);

  cluster
      ->GetCursor<Row>(kMasterHost, cluster.GetDeadline(), 42,
                       table.FormatWithTableName("SELECT Id, Value FROM {}"))
      .ForEach([&db_rows](Row&& row) { db_rows.push_back(std::move(row)); },
               cluster.GetDeadline());

  auto asd = table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<Row>();

  ASSERT_EQ(db_rows.size(), rows_to_insert.size());
  EXPECT_EQ(db_rows, rows_to_insert);
  ASSERT_EQ(asd.size(), rows_to_insert.size());

  /*const auto deadline =
  engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  auto stream =
      cluster.Select(kMasterHost, deadline, "SELECT Id, Value FROM test")
          .AsStreamOf<Row>();

  std::move(stream).ForEach(
      [](Row&&) {
        // std::cout << row.id << " " << row.value << std::endl;
      },
      1, deadline);*/
}

/*UTEST(Cluster, DISABLED_BigInsert) {
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
}*/

UTEST(ShowCase, Basic) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, "Id INT, Value TEXT, CreatedAt DATETIME(6)"};

  const auto deadline = cluster.GetDeadline();

  struct Row final {
    std::int32_t id{};
    std::string value;
    std::chrono::system_clock::time_point created_at;
  };
  const std::vector<Row> rows =
      cluster
          ->Execute(
              ClusterHostType::kMaster, deadline,
              table.FormatWithTableName(
                  "SELECT Id, Value, CreatedAt FROM {} WHERE CreatedAt > ?"),
              std::chrono::system_clock::now() - std::chrono::hours{3})
          .AsVector<Row>();
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
