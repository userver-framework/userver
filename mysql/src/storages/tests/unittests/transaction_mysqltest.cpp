#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

struct Row final {
  std::int32_t id{};
  std::string value;

  bool operator==(const Row& other) const {
    return id == other.id && value == other.value;
  }
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

 private:
  TableMeta() : table{"Id INT NOT NULL, Value TEXT NOT NULL"} {}
};

}  // namespace

UTEST_DEATH(TransactionDeathTest, AccessAfterCommit) {
  auto meta = TableMeta::Create();

  auto transaction = meta.table.Begin();
  transaction.Commit();

  EXPECT_UINVARIANT_FAILURE(transaction.Execute(meta.GetSelectQuery()));
}

UTEST_DEATH(TransactionDeathTest, AccessAfterRollback) {
  auto meta = TableMeta::Create();

  auto transaction = meta.table.Begin();
  transaction.Rollback();

  EXPECT_UINVARIANT_FAILURE(transaction.Execute(meta.GetSelectQuery()));
}

UTEST(Transaction, Commit) {
  auto meta = TableMeta::Create();

  {
    auto transaction = meta.table.Begin();
    transaction.Execute(meta.GetInsertQuery(), 1, "some text");
    transaction.Commit();
  }

  EXPECT_NO_THROW(meta.table.DefaultExecute(TableMeta::GetDefaultSelectQuery())
                      .AsSingleRow<Row>());
}

UTEST(Transaction, Rollback) {
  auto meta = TableMeta::Create();

  {
    auto transaction = meta.table.Begin();
    transaction.Execute(meta.GetInsertQuery(), 1, "some text");
    EXPECT_NO_THROW(
        transaction.Execute(meta.GetSelectQuery()).AsSingleRow<Row>());
    transaction.Rollback();
  }

  auto row_opt = meta.table.DefaultExecute(TableMeta::GetDefaultSelectQuery())
                     .AsOptionalSingleRow<Row>();
  EXPECT_FALSE(row_opt.has_value());
}

UTEST(Transaction, AutoRollback) {
  auto meta = TableMeta::Create();

  {
    auto transaction = meta.table.Begin();
    transaction.Execute(meta.GetInsertQuery(), 1, "some string");
    EXPECT_NO_THROW(
        transaction.Execute(meta.GetSelectQuery()).AsSingleRow<Row>());
  }

  auto row_opt = meta.table.DefaultExecute(TableMeta::GetDefaultSelectQuery())
                     .AsOptionalSingleRow<Row>();
  EXPECT_FALSE(row_opt.has_value());
}

UTEST(Transaction, ActuallyTransactional) {
  auto meta = TableMeta::Create();

  {
    auto insert_transaction = meta.table.Begin();
    insert_transaction.Execute(meta.GetInsertQuery(), 5,
                               "i am not seen by another transaction");

    const auto select = [&meta] {
      auto transaction = meta.table.Begin();
      return transaction.Execute(meta.GetSelectQuery())
          .AsOptionalSingleRow<Row>();
    };

    EXPECT_FALSE(select().has_value());

    insert_transaction.Commit();
    EXPECT_TRUE(select().has_value());
  }
}

UTEST(Transaction, InsertOne) {
  auto meta = TableMeta::Create();

  {
    const Row row{1, "some text"};

    auto transaction = meta.table.Begin();
    transaction.ExecuteDecompose(meta.GetInsertQuery(), row);

    EXPECT_EQ(transaction.Execute(meta.GetSelectQuery()).AsSingleRow<Row>(),
              row);

    transaction.Rollback();
    EXPECT_FALSE(meta.table.DefaultExecute(meta.GetDefaultSelectQuery())
                     .AsOptionalSingleRow<Row>()
                     .has_value());
  }
}

UTEST(Transaction, InsertMany) {
  auto meta = TableMeta::Create();

  {
    const std::vector<Row> rows{{1, "some text"}, {2, "other text"}};

    auto transaction = meta.table.Begin();
    transaction.ExecuteBulk(meta.GetInsertQuery(), rows);

    EXPECT_EQ(transaction.Execute(meta.GetSelectQuery()).AsVector<Row>(), rows);

    transaction.Rollback();
    EXPECT_FALSE(meta.table.DefaultExecute(meta.GetDefaultSelectQuery())
                     .AsOptionalSingleRow<Row>()
                     .has_value());
  }
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
