#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

#include <userver/storages/mysql/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct DbRow final {
  std::int32_t a;
  std::int32_t b;
};
struct UserRow final {
  std::int64_t mult;
};

}  // namespace

namespace storages::mysql::convert {

UserRow Convert(DbRow&& db_row, To<UserRow>) {
  return {static_cast<std::int64_t>(db_row.a) * db_row.b};
}

int Convert(std::string&& field, To<int>) { return std::stoi(field); }

}  // namespace storages::mysql::convert

namespace storages::mysql::tests {

UTEST(MappedResultSet, Works) {
  TmpTable table{"a INT NOT NULL, b INT NOT NULL"};
  table.DefaultExecute("INSERT INTO {} VALUES(?, ?)", 2, 3);

  const auto db_row = table.DefaultExecute("SELECT a, b FROM {}")
                          .MapFrom<DbRow>()
                          .AsVector<UserRow>();

  ASSERT_EQ(db_row.size(), 1);
  EXPECT_EQ(db_row.front().mult, 2 * 3);
}

UTEST(MappedResultSet, FieldWorks) {
  TmpTable table{"text TEXT NOT NULL"};

  const std::string text = "hi from FieldTag";
  table.DefaultExecute("INSERT INTO {} VALUES(?)", text);

  const auto container = table.DefaultExecute("SELECT text FROM {}")
                             .AsVector<std::string>(kFieldTag);
  ASSERT_EQ(container.size(), 1);
  EXPECT_EQ(container.front(), text);
}

namespace {

struct DbUser final {
  std::string first_name;
  std::string last_name;
};

std::string Convert(DbUser&& db_user,
                    storages::mysql::convert::To<std::string>) {
  return fmt::format("{} {}", db_user.first_name, db_user.last_name);
}

}  // namespace

UTEST(MappedResultSet, MappedVectorWorks) {
  ClusterWrapper cluster{};
  cluster->ExecuteCommand(ClusterHostType::kMaster,
                          "DROP TABLE IF EXISTS Users");
  cluster->ExecuteCommand(
      ClusterHostType::kMaster,
      "CREATE TABLE Users(first_name TEXT NOT NULL, last_name TEXT NOT NULL)");

  cluster->ExecuteDecompose(ClusterHostType::kMaster,
                            "INSERT INTO Users VALUES(?, ?)",
                            DbUser{"Ivan", "Trofimov"});

  const auto users = cluster
                         ->Execute(ClusterHostType::kMaster,
                                   "SELECT first_name, last_name FROM Users")
                         .MapFrom<DbUser>()
                         .AsVector<std::string>();
  ASSERT_EQ(users.size(), 1);
  EXPECT_EQ(users.front(), "Ivan Trofimov");
}

UTEST(MappedResultSet, MappedFieldWorks) {
  TmpTable table{"Value TEXT NOT NULL"};
  table.DefaultExecute("INSERT INTO {} VALUES(?)", "123");

  const auto as_int = table.DefaultExecute("SELECT Value FROM {}")
                          .MapFrom<std::string>()
                          .AsVector<int>(kFieldTag);

  ASSERT_EQ(as_int.size(), 1);
  EXPECT_EQ(as_int.front(), 123);
}

UTEST(ResultSet, UnmappedFieldWorks) {
  ClusterWrapper cluster{};
  const auto res =
      cluster.DefaultExecute("SELECT 'field'").AsSingleField<std::string>();
  EXPECT_EQ(res, "field");
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
