#include "utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

// TODO : think about this
UTEST(PreparedStatement, Invalidation) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, "(Id INT, Value TEXT)"};

  const auto select_query = table.FormatWithTableName("SELECT * FROM {}");
  cluster.DefaultExecute(select_query);

  cluster.DefaultExecute(
      table.FormatWithTableName("ALTER TABLE {} ADD COLUMN tp DATETIME(6)"));

  EXPECT_ANY_THROW(cluster.DefaultExecute(select_query));
  cluster.DefaultExecute(select_query);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
