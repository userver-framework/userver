#include <userver/utest/utest.hpp>

#include <iostream>

#include <userver/storages/mysql/cluster.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Row final {
  int id{};
  std::string value;
};

}  // namespace

UTEST(Cluster, TypedWorks) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  storages::mysql::Cluster cluster{};

  auto rows = cluster.Execute<Row>(
      deadline, "SELECT Id, Value FROM test WHERE Id=? OR Value=?", 1, "two");

  for (const auto& [id, value] : rows) {
    std::cout << id << ": " << value << std::endl;
  }
}

USERVER_NAMESPACE_END
