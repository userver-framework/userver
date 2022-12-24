#include <benchmark/benchmark.h>
#include "../utils_mysqltest.hpp"

#include <userver/engine/run_standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::benches {

void insert_retrieve(benchmark::State& state) {
  engine::RunStandalone([&state] {
    tests::ClusterWrapper cluster{};

    const auto rows_count = state.range(0);

    struct Row final {
      std::int32_t id{};
      std::string value;
    };
    std::vector<Row> rows_to_insert;
    rows_to_insert.reserve(rows_count);
    for (int i = 0; i < rows_count; ++i) {
      rows_to_insert.push_back({i, "some string, i don't really care"});
    }

    for (auto _ : state) {
      state.PauseTiming();
      tests::TmpTable table{"Id INT NOT NULL PRIMARY KEY, Value TEXT NOT NULL"};
      state.ResumeTiming();

      cluster->InsertMany(
          table.FormatWithTableName("INSERT INTO {} VALUES(?, ?)"),
          rows_to_insert);

      const auto db_rows =
          table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<Row>();

      state.PauseTiming();
      table.DefaultExecute("DROP TABLE {}");
      if (rows_to_insert.size() != db_rows.size()) {
        state.SkipWithError("oops");
      }
    }
  });
}
BENCHMARK(insert_retrieve)->Range(1 << 10, 1 << 20)->RangeMultiplier(4);

}  // namespace storages::mysql::benches

USERVER_NAMESPACE_END
