#include <userver/utest/utest.hpp>

#include <optional>
#include <string>
#include <vector>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithArrays final {
  std::vector<std::vector<uint64_t>> array_of_ints;
  std::vector<int32_t> key;
};

struct DataOfArrays final {
  uint64_t value;
  std::vector<std::string> data;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithArrays> {
  using mapped_type = std::tuple<columns::ArrayColumn<columns::UInt64Column>,
                                 columns::Int32Column>;
};

template <>
struct CppToClickhouse<DataOfArrays> {
  using mapped_type = std::tuple<columns::UInt64Column,
                                 columns::ArrayColumn<columns::StringColumn>>;
};

}  // namespace storages::clickhouse::io

UTEST(Array, Works) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
      "(value Array(UInt64), key Int32)");

  const auto insertion_data =
      DataWithArrays{{{1, 2, 3, 4}, {1}, {}}, {1, 2, 3}};
  cluster->Insert("tmp_table", {"value", "key"}, insertion_data);

  const storages::clickhouse::Query q{
      "SELECT value, key FROM tmp_table ORDER BY key"};
  const auto res = cluster->Execute(q).As<DataWithArrays>();
  EXPECT_EQ(res.array_of_ints.size(), 3);
  EXPECT_EQ(res.array_of_ints[0], std::vector<uint64_t>({1, 2, 3, 4}));
  EXPECT_EQ(res.key[0], 1);
  EXPECT_EQ(res.array_of_ints[1], std::vector<uint64_t>{1});
  EXPECT_EQ(res.key[1], 2);
  EXPECT_EQ(res.array_of_ints[2], std::vector<uint64_t>{});
  EXPECT_EQ(res.key[2], 3);
}

UTEST(Array, IterationWorks) {
  ClusterWrapper cluster{};
  auto res = cluster
                 ->Execute(
                     "SELECT c.number, arrayMap(x->toString(x), range(0, "
                     "c.number)) FROM "
                     "system.numbers c LIMIT 10")
                 .AsRows<DataOfArrays>();
  size_t ind = 0;
  for (auto it = res.begin(); it != res.end(); it++, ++ind) {
    ASSERT_EQ(it->value, ind);
    ASSERT_EQ(it->data.size(), ind);
    for (size_t count = 0; count < ind; ++count) {
      ASSERT_EQ(it->data[count], std::to_string(count));
    }
  }
  ASSERT_EQ(ind, 10);

  ind = 0;
  for (auto it = res.begin(); it != res.end(); it++, ++ind) {
    ASSERT_EQ(it->value, ind);
    ASSERT_EQ(it->data.size(), ind);
    for (size_t count = 0; count < ind; ++count) {
      ASSERT_EQ(it->data[count], std::to_string(count));
    }
  }
  ASSERT_EQ(ind, 10);

  ind = 0;
  for (auto it = res.begin(); it != res.end(); it++, ++ind) {
    ASSERT_EQ(it->value, ind);
    ASSERT_EQ(it->data.size(), ind);
    for (size_t count = 0; count < ind; ++count) {
      ASSERT_EQ(it->data[count], std::to_string(count));
    }
  }
  ASSERT_EQ(ind, 10);
}

USERVER_NAMESPACE_END
