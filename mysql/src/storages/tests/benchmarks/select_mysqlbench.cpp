#include <benchmark/benchmark.h>
#include "../utils_mysqltest.hpp"

#include <userver/engine/run_standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::benches {

void select(benchmark::State& state) {
  engine::RunStandalone([&state] {
    tests::ClusterWrapper cluster;
    tests::TmpTable table{cluster, "Id int NOT NULL, Value TEXT NOT NULL"};
    for (int i = 0; i < state.range(0); ++i) {
      table.DefaultExecute("INSERT INTO {} VALUES(?, ?)", i,
                           "some moderate size string");
    }

    struct Row final {
      std::int32_t id{};
      std::string value;
    };

    const auto query = table.FormatWithTableName("SELECT Id, Value FROM {}");
    const engine::Deadline deadline{};
    for (auto _ : state) {
      const auto rows =
          cluster->Select(ClusterHostType::kMaster, deadline, query)
              .AsVector<Row>();
    }
  });
}

BENCHMARK(select)->Range(1, 256)->RangeMultiplier(2);

}  // namespace storages::mysql::benches

USERVER_NAMESPACE_END
