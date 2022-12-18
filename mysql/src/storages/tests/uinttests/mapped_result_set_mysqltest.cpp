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

UserRow MySQLConvert(DbRow&& db_row, To<UserRow>) {
  return {static_cast<std::int64_t>(db_row.a) * db_row.b};
}

}  // namespace storages::mysql::convert

namespace storages::mysql::tests {

UTEST(MappedResultSet, Works) {
  TmpTable table{"a INT NOT NULL, b INT NOT NULL"};
  table.DefaultExecute("INSERT INTO {} VALUES(?, ?)", 2, 3);

  const auto db_row = table.DefaultExecute("SELECT a, b FROM {}")
                          .AsVectorMapped<UserRow, DbRow>();

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

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
