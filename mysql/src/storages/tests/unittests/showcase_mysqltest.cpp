#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

UTEST(ShowCase, MainPage) {
  ClusterWrapper cluster{};
  cluster->ExecuteCommand(ClusterHostType::kPrimary,
                          "DROP TABLE IF EXISTS Devs");
  cluster->ExecuteCommand(ClusterHostType::kPrimary,
                          "CREATE TABLE Devs(id VARCHAR(32) PRIMARY KEY, "
                          "loc_written BIGINT UNSIGNED NOT NULL)");

  /// [uMySQL usage sample - main page]
  struct Developer final {
    std::string developer_id;
    std::uint64_t lines_of_code_written{};  // uint64_t is generous :)
  };
  std::vector<Developer> best_developers =
      cluster
          ->Execute(ClusterHostType::kPrimary,
                    "SELECT id, loc_written FROM Devs "
                    "ORDER BY loc_written DESC LIMIT ?",
                    3)
          .AsVector<Developer>();
  // your top 3 best devs are right here!
  /// [uMySQL usage sample - main page]
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
