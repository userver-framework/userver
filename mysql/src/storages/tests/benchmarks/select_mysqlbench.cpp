#include <benchmark/benchmark.h>
#include "../utils_mysqltest.hpp"

#include <userver/engine/run_standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::benches {

void select(benchmark::State& state) {
  engine::RunStandalone([&state] {
    tests::ClusterWrapper cluster;
    tests::TmpTable table{cluster, "Id INT NOT NULL, Value TEXT NOT NULL"};
    for (int i = 0; i < state.range(0); ++i) {
      table.DefaultExecute("INSERT INTO {} VALUES(?, ?)", i,
                           "some moderate size string");
    }

    struct Row final {
      std::int32_t id{};
      std::string value;
    };

    const auto query = table.FormatWithTableName("SELECT Id, Value FROM {}");
    for (auto _ : state) {
      const auto rows =
          cluster->Execute(ClusterHostType::kPrimary, query).AsVector<Row>();
    }
  });
}

BENCHMARK(select)->Range(1, 256)->RangeMultiplier(2);

void select_many_small_columns(benchmark::State& state) {
  engine::TaskProcessorPoolsConfig config{};
  config.defer_events = false;

  engine::RunStandalone(1, config, [&state] {
    tests::ClusterWrapper cluster;
    tests::TmpTable table{
        cluster,
        "a INT NOT NULL, b INT NOT NULL, c INT NOT NULL, d INT NOT NULL, "
        "e INT NOT NULL, f INT NOT NULL, g INT NOT NULL, h INT NOT NULL, "
        "j INT NOT NULL, k INT NOT NULL"};
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

    cluster->ExecuteBulk(
        ClusterHostType::kPrimary,
        table.FormatWithTableName(
            "INSERT INTO {} VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
        rows_to_insert);

    const storages::mysql::Query query = table.FormatWithTableName(
        "SELECT a, b, c, d, e, f, g, h, j, k FROM {}");
    for (auto _ : state) {
      const auto rows =
          cluster->Execute(ClusterHostType::kPrimary, query).AsVector<Row>();

      state.PauseTiming();
      if (rows_to_insert != rows) {
        state.SkipWithError("SELECT OR INSERT IS BROKEN");
      }
      state.ResumeTiming();
    }
  });
}
BENCHMARK(select_many_small_columns)->Range(1000, 100'000);

}  // namespace storages::mysql::benches

USERVER_NAMESPACE_END
