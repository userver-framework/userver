#include <userver/utest/utest.hpp>

#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithTwoColumns final {
  std::vector<uint64_t> numbers;
  std::vector<std::string> strings;
};

struct DataWithTwoFields final {
  uint64_t number;
  std::string string;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithTwoColumns> {
  using mapped_type = std::tuple<columns::UInt64Column, columns::StringColumn>;
};

template <>
struct CppToClickhouse<DataWithTwoFields> {
  using mapped_type = std::tuple<columns::UInt64Column, columns::StringColumn>;
};

}  // namespace storages::clickhouse::io

UTEST_DEATH(ColumnsMismatchDeathTest, Insert) {
  ClusterWrapper cluster{};

  const DataWithTwoColumns data{};
  EXPECT_UINVARIANT_FAILURE(cluster->Insert("some_table", {"one"}, data));
  EXPECT_UINVARIANT_FAILURE(
      cluster->Insert("some_table", {"one", "two", "three"}, data));
}

UTEST_DEATH(ColumnsMismatchDeathTest, Select) {
  ClusterWrapper cluster{};

  const storages::clickhouse::Query more_columns_query{
      "SELECT toUInt64(0) as one, randomString(2) as two, randomString(2) as "
      "three"};
  EXPECT_UINVARIANT_FAILURE(
      cluster->Execute(more_columns_query).As<DataWithTwoColumns>());
  EXPECT_UINVARIANT_FAILURE(
      cluster->Execute(more_columns_query).AsRows<DataWithTwoFields>());
  EXPECT_UINVARIANT_FAILURE(cluster->Execute(more_columns_query)
                                .AsContainer<std::vector<DataWithTwoFields>>());

  const storages::clickhouse::Query less_columns_query{
      "SELECT toUInt64(0) as one"};
  EXPECT_UINVARIANT_FAILURE(
      cluster->Execute(less_columns_query).As<DataWithTwoColumns>());
  EXPECT_UINVARIANT_FAILURE(
      cluster->Execute(less_columns_query).AsRows<DataWithTwoFields>());
  EXPECT_UINVARIANT_FAILURE(cluster->Execute(less_columns_query)
                                .AsContainer<std::vector<DataWithTwoFields>>());
}

USERVER_NAMESPACE_END
