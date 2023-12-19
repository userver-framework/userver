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
  cluster->ExecuteCommand(ClusterHostType::kPrimary,
                          "DROP TABLE IF EXISTS Users");
  cluster->ExecuteCommand(
      ClusterHostType::kPrimary,
      "CREATE TABLE Users(first_name TEXT NOT NULL, last_name TEXT NOT NULL)");

  cluster->ExecuteDecompose(ClusterHostType::kPrimary,
                            "INSERT INTO Users VALUES(?, ?)",
                            DbUser{"Ivan", "Trofimov"});

  const auto users = cluster
                         ->Execute(ClusterHostType::kPrimary,
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

namespace {

struct IdAndValue final {
  std::int32_t id{};
  std::string value;

  bool operator==(const IdAndValue& other) const noexcept {
    return id == other.id && value == other.value;
  }
};

std::pair<const std::int32_t, IdAndValue> Convert(
    IdAndValue&& db_value,
    storages::mysql::convert::To<std::pair<const std::int32_t, IdAndValue>>) {
  const auto id = db_value.id;

  return {id, std::move(db_value)};
}

}  // namespace

UTEST(MappedResultSet, WorksForUnorderedMap) {
  TmpTable table{"Id INT NOT NULL, Value TEXT NOT NULL"};
  for (std::int32_t i = 1; i <= 2; ++i) {
    table.DefaultExecute("INSERT INTO {} VALUES(?, ?)", i, std::to_string(i));
  }

  using Map = std::unordered_map<std::int32_t, IdAndValue>;

  const auto container = table.DefaultExecute("SELECT Id, Value FROM {}")
                             .MapFrom<IdAndValue>()
                             .AsContainer<Map>();

  const Map expected{{1, {1, "1"}}, {2, {2, "2"}}};

  EXPECT_EQ(container, expected);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
