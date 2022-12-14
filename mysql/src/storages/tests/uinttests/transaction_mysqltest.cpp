#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

struct Row final {
  std::int32_t id{};
  std::string value;
};

struct TableMeta final {
  static TableMeta Create() { return TableMeta{}; };

  std::string GetInsertQuery() const {
    return table.FormatWithTableName("INSERT INTO {} VALUES (?, ?)");
  }

  std::string GetSelectQuery() const {
    return table.FormatWithTableName(GetDefaultSelectQuery());
  }

  static std::string GetDefaultSelectQuery() {
    return "SELECT Id, Value FROM {}";
  };

  TmpTable table;
  const engine::Deadline deadline;

 private:
  TableMeta()
      : table{"Id INT NOT NULL, Value TEXT NOT NULL"},
        deadline{table.GetDeadline()} {}
};

}  // namespace

UTEST_DEATH(TransactionDeathTest, AccessAfterCommit) {
  auto meta = TableMeta::Create();

  auto transaction = meta.table.Begin();
  transaction.Commit(meta.deadline);

  EXPECT_UINVARIANT_FAILURE(
      transaction.Execute(meta.deadline, meta.GetSelectQuery()));
}

UTEST_DEATH(TransactionDeathTest, AccessAfterRollback) {
  auto meta = TableMeta::Create();

  auto transaction = meta.table.Begin();
  transaction.Rollback(meta.deadline);

  EXPECT_UINVARIANT_FAILURE(
      transaction.Execute(meta.deadline, meta.GetSelectQuery()));
}

UTEST(Transaction, Commit) {
  auto meta = TableMeta::Create();

  {
    auto transaction = meta.table.Begin();
    transaction.Execute(meta.deadline, meta.GetInsertQuery(), 1, "some text");
    transaction.Commit(meta.deadline);
  }

  EXPECT_NO_THROW(meta.table.DefaultExecute(TableMeta::GetDefaultSelectQuery())
                      .AsSingleRow<Row>());
}

UTEST(Transaction, Rollback) {
  auto meta = TableMeta::Create();

  {
    auto transaction = meta.table.Begin();
    transaction.Execute(meta.deadline, meta.GetInsertQuery(), 1, "some text");
    EXPECT_NO_THROW(transaction.Execute(meta.deadline, meta.GetSelectQuery())
                        .AsSingleRow<Row>());
    transaction.Rollback(meta.deadline);
  }

  auto row_opt = meta.table.DefaultExecute(TableMeta::GetDefaultSelectQuery())
                     .AsOptionalSingleRow<Row>();
  EXPECT_FALSE(row_opt.has_value());
}

UTEST(Transaction, AutoRollback) {
  auto meta = TableMeta::Create();

  {
    auto transaction = meta.table.Begin();
    transaction.Execute(meta.deadline, meta.GetInsertQuery(), 1, "some string");
    EXPECT_NO_THROW(transaction.Execute(meta.deadline, meta.GetSelectQuery())
                        .AsSingleRow<Row>());
  }

  auto row_opt = meta.table.DefaultExecute(TableMeta::GetDefaultSelectQuery())
                     .AsOptionalSingleRow<Row>();
  EXPECT_FALSE(row_opt.has_value());
}

UTEST(Transaction, ActuallyTransactional) {
  auto meta = TableMeta::Create();

  {
    auto transaction = meta.table.Begin();
    transaction.Execute(meta.deadline, meta.GetInsertQuery(), 5,
                        "i am not seen by another connection");

    const auto outside_select = [&meta] {
      return meta.table.DefaultExecute(TableMeta::GetDefaultSelectQuery())
          .AsOptionalSingleRow<Row>();
    };

    EXPECT_FALSE(outside_select().has_value());

    transaction.Commit(meta.deadline);
    EXPECT_TRUE(outside_select().has_value());
  }
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
