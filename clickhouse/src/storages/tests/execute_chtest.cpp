#include <userver/utest/utest.hpp>

#include <userver/storages/clickhouse/query.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct Data final {
  std::vector<uint64_t> numbers;
  std::vector<std::string> strings;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<Data> final {
  using mapped_type = std::tuple<columns::UInt64Column, columns::StringColumn>;
};

}  // namespace storages::clickhouse::io

UTEST(Execute, MappingWorks) {
  ClusterWrapper cluster{};

  storages::clickhouse::Query q{
      "SELECT c.number, randomString(10), c.number as t, NOW() "
      "FROM "
      "numbers(0, 100000) c "};
  auto res = cluster->Execute(q).As<Data>();
}

USERVER_NAMESPACE_END
