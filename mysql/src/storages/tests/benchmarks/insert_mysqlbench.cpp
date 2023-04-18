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

      cluster->ExecuteBulk(
          ClusterHostType::kPrimary,
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

void batch_insert(benchmark::State& state) {
  engine::TaskProcessorPoolsConfig config{};
  config.defer_events = false;

  engine::RunStandalone(1, config, [&state] {
    tests::ClusterWrapper cluster;
    struct Row final {
      std::int32_t a{};
      std::int32_t b{};
      std::int32_t c{};
      std::int32_t d{};
      std::int32_t e{};
      std::int32_t f{};
      std::int32_t g{};
      std::int32_t h{};
      std::int32_t j{};
      std::int32_t k{};

      bool operator==(const Row& other) const {
        return boost::pfr::structure_tie(*this) ==
               boost::pfr::structure_tie(other);
      }
    };
    std::vector<Row> rows_to_insert;
    rows_to_insert.reserve(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
      auto& row = rows_to_insert.emplace_back();
      boost::pfr::for_each_field(row, [i](auto& field) { field = i; });
    }

    for (auto _ : state) {
      state.PauseTiming();
      tests::TmpTable table{
          cluster,
          "a INT NOT NULL, b INT NOT NULL, c INT NOT NULL, d INT NOT NULL, "
          "e INT NOT NULL, f INT NOT NULL, g INT NOT NULL, h INT NOT NULL, "
          "j INT NOT NULL, k INT NOT NULL"};
      const storages::mysql::Query query{
          table.FormatWithTableName(
              "INSERT INTO {} VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
      };
      state.ResumeTiming();

      cluster->ExecuteBulk(ClusterHostType::kPrimary, query, rows_to_insert);

      state.PauseTiming();
      const auto db_rows =
          table.DefaultExecute("SELECT a, b, c, d, e, f, g, h, j, k FROM {}")
              .AsVector<Row>();
      if (rows_to_insert != db_rows) {
        state.SkipWithError("INSERT OR SELECT IS BROKEN");
      }
      table.DefaultExecute("DROP TABLE {}");
      state.ResumeTiming();
    }
  });
}
BENCHMARK(batch_insert)->Range(1000, 100'000);

}  // namespace storages::mysql::benches

USERVER_NAMESPACE_END
