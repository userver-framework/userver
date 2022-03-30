#include <userver/utest/utest.hpp>

#include <optional>
#include <string>
#include <vector>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithNulls final {
  std::vector<std::optional<uint64_t>> nullable_ints;
  std::vector<int32_t> key;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithNulls> {
  using mapped_type = std::tuple<columns::NullableColumn<columns::UInt64Column>,
                                 columns::Int32Column>;
};

}  // namespace storages::clickhouse::io

UTEST(SelectNullable, Works) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
      "(value Nullable(UInt64), key Int32)");

  const auto insertion_data =
      DataWithNulls{{std::nullopt, 1, std::nullopt}, {1, 2, 3}};
  cluster->Insert("tmp_table", {"value", "key"}, insertion_data);

  const storages::clickhouse::Query q{
      "SELECT value, key FROM tmp_table ORDER BY key"};
  const auto res = cluster->Execute(q).As<DataWithNulls>();
  EXPECT_EQ(res.nullable_ints.size(), 3);
  EXPECT_EQ(res.nullable_ints[0], std::nullopt);
  EXPECT_EQ(res.key[0], 1);
  EXPECT_EQ(res.nullable_ints[1], 1);
  EXPECT_EQ(res.key[1], 2);
  EXPECT_EQ(res.nullable_ints[2], std::nullopt);
  EXPECT_EQ(res.key[2], 3);
}

USERVER_NAMESPACE_END
