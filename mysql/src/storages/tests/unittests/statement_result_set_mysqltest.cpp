#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

void PrepareSampleTable(const Cluster& cluster) {
  cluster.ExecuteCommand(ClusterHostType::kPrimary,
                         "DROP TABLE IF EXISTS SampleTable");
  cluster.ExecuteCommand(
      ClusterHostType::kPrimary,
      "CREATE TABLE SampleTable(id INT NOT NULL, value TEXT NOT NULL)");
}

}  // namespace

namespace as_vector_sample {

/// [uMySQL usage sample - StatementResultSet AsVector]
struct SampleRow final {
  std::int32_t id;
  std::string value;

  bool operator==(const SampleRow& other) const {
    return id == other.id && value == other.value;
  }
};

void PerformAsVector(const Cluster& cluster,
                     const std::vector<SampleRow>& expected_data) {
  cluster.ExecuteBulk(ClusterHostType::kPrimary,
                      "INSERT INTO SampleTable(id, value) VALUES (?, ?)",
                      expected_data);

  const auto db_rows = cluster
                           .Execute(ClusterHostType::kPrimary,
                                    "SELECT id, value FROM SampleTable")
                           .AsVector<SampleRow>();

  static_assert(
      std::is_same_v<const std::vector<SampleRow>, decltype(db_rows)>);
  EXPECT_EQ(expected_data, db_rows);
}
/// [uMySQL usage sample - StatementResultSet AsVector]

UTEST(StatementResultSet, AsVector) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  const std::vector<SampleRow> rows{{1, "first"}, {2, "second"}};
  PerformAsVector(*cluster, rows);
}

}  // namespace as_vector_sample

namespace as_vector_field_sample {
/// [uMySQL usage sample - StatementResultSet AsVectorFieldTag]
void PerformAsVectorFieldTag(const Cluster& cluster) {
  const auto vector_of_field_values =
      cluster
          .Execute(ClusterHostType::kPrimary, "SELECT value FROM SampleTable")
          .AsVector<std::string>(kFieldTag);

  static_assert(std::is_same_v<const std::vector<std::string>,
                               decltype(vector_of_field_values)>);
}
/// [uMySQL usage sample - StatementResultSet AsVectorFieldTag]

UTEST(StatementResultSet, AsVectorFieldTag) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  PerformAsVectorFieldTag(*cluster);
}

}  // namespace as_vector_field_sample

namespace as_single_row_sample {

/// [uMySQL usage sample - StatementResultSet AsSingleRow]
struct SampleRow final {
  std::int32_t id;
  std::string value;

  bool operator==(const SampleRow& other) const {
    return id == other.id && value == other.value;
  }
};

void PerformAsSingleRow(const Cluster& cluster, const SampleRow& data) {
  cluster.ExecuteDecompose(ClusterHostType::kPrimary,
                           "INSERT INTO SampleTable(id, value) VALUES (?, ?)",
                           data);

  const auto db_row = cluster
                          .Execute(ClusterHostType::kPrimary,
                                   "SELECT id, value FROM SampleTable")
                          .AsSingleRow<SampleRow>();
  EXPECT_EQ(data, db_row);
}
/// [uMySQL usage sample - StatementResultSet AsSingleRow]

UTEST(StatementResultSet, AsSingleRow) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  PerformAsSingleRow(*cluster, {1, "first"});
}

}  // namespace as_single_row_sample

namespace as_single_field_sample {

/// [uMySQL usage sample - StatementResultSet AsSingleField]
void PerformAsSingleField(const Cluster& cluster) {
  cluster.Execute(ClusterHostType::kPrimary,
                  "INSERT INTO SampleTable(id, value) VALUES (?, ?)",  //
                  1, "some value");

  const auto value =
      cluster
          .Execute(ClusterHostType::kPrimary, "SELECT value FROM SampleTable")
          .AsSingleField<std::string>();
  EXPECT_EQ(value, "some value");
}
/// [uMySQL usage sample - StatementResultSet AsSingleField]

UTEST(StatementResultSet, AsSingleField) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  PerformAsSingleField(*cluster);
}

}  // namespace as_single_field_sample

namespace as_container_sample {

/// [uMySQL usage sample - StatementResultSet AsContainer]
struct SampleRow final {
  std::int32_t id;
  std::string value;

  bool operator<(const SampleRow& other) const {
    return id < other.id || (id == other.id && value < other.value);
  }
  bool operator==(const SampleRow& other) const {
    return id == other.id && value == other.value;
  }
};

void PerformAsContainer(const Cluster& cluster,
                        const std::vector<SampleRow>& data) {
  cluster.ExecuteBulk(ClusterHostType::kPrimary,
                      "INSERT INTO SampleTable(id, value) VALUES(?, ?)", data);

  const auto set_or_rows = cluster
                               .Execute(ClusterHostType::kPrimary,
                                        "SELECT id, value FROM SampleTable")
                               .AsContainer<std::set<SampleRow>>();

  static_assert(
      std::is_same_v<const std::set<SampleRow>, decltype(set_or_rows)>);
  const auto input_as_set = std::set<SampleRow>{data.begin(), data.end()};
  EXPECT_EQ(set_or_rows, input_as_set);
}
/// [uMySQL usage sample - StatementResultSet AsContainer]

UTEST(StatementResultSet, AsContainer) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  PerformAsContainer(*cluster, {{1, "first"}, {2, "second"}});
}

}  // namespace as_container_sample

namespace as_container_field_sample {

/// [uMySQL usage sample - StatementResultSet AsContainerFieldTag]
void PerformAsContainerField(const Cluster& cluster) {
  cluster.Execute(ClusterHostType::kPrimary,
                  "INSERT INTO SampleTable(id, value) VALUES(?, ?)", 1,
                  "some value");

  const auto set_of_values =
      cluster
          .Execute(ClusterHostType::kPrimary, "SELECT value FROM SampleTable")
          .AsContainer<std::set<std::string>>(kFieldTag);

  static_assert(
      std::is_same_v<const std::set<std::string>, decltype(set_of_values)>);
  ASSERT_EQ(set_of_values.size(), 1);
  EXPECT_EQ(*set_of_values.begin(), "some value");
}
/// [uMySQL usage sample - StatementResultSet AsContainerFieldTag]

UTEST(StatementResultSet, AsContainerFieldTag) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  PerformAsContainerField(*cluster);
}

}  // namespace as_container_field_sample

namespace as_optional_single_row_sample {

/// [uMySQL usage sample - StatementResultSet AsOptionalSingleRow]
struct SampleRow final {
  std::int32_t id;
  std::string value;
};

void PerformAsOptionalSingleRow(const Cluster& cluster) {
  const auto row_optional = cluster
                                .Execute(ClusterHostType::kPrimary,
                                         "SELECT id, value FROM SampleTable")
                                .AsOptionalSingleRow<SampleRow>();

  static_assert(
      std::is_same_v<const std::optional<SampleRow>, decltype(row_optional)>);
  EXPECT_FALSE(row_optional.has_value());
}
/// [uMySQL usage sample - StatementResultSet AsOptionalSingleRow]

UTEST(StatementResultSet, AsOptionalSingleRow) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  PerformAsOptionalSingleRow(*cluster);
}

}  // namespace as_optional_single_row_sample

namespace as_optional_single_field_sample {

/// [uMySQL usage sample - StatementResultSet AsOptionalSingleField]
void PerformAsOptionalSingleField(const Cluster& cluster) {
  const auto field_optional =
      cluster
          .Execute(ClusterHostType::kPrimary, "SELECT value FROM SampleTable")
          .AsOptionalSingleField<std::string>();

  static_assert(std::is_same_v<const std::optional<std::string>,
                               decltype(field_optional)>);
  EXPECT_FALSE(field_optional.has_value());
}
/// [uMySQL usage sample - StatementResultSet AsOptionalSingleField]

UTEST(StatementResultSet, AsOptionalSingleField) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  PerformAsOptionalSingleField(*cluster);
}

}  // namespace as_optional_single_field_sample

namespace map_from_sample {

/// [uMySQL usage sample - StatementResultSet MapFrom]
struct SampleRow final {
  std::int32_t id;
  std::string value;

  bool operator==(const SampleRow& other) const {
    return id == other.id && value == other.value;
  }
};

std::pair<const std::int32_t, std::string> Convert(
    SampleRow&& db_row,
    storages::mysql::convert::To<std::pair<const std::int32_t, std::string>>) {
  return {db_row.id, std::move(db_row.value)};
}

void PerformMapFrom(const Cluster& cluster,
                    const std::vector<SampleRow>& data) {
  cluster.ExecuteBulk(ClusterHostType::kPrimary,
                      "INSERT INTO SampleTable(id, value) VALUES(?, ?)", data);

  const auto id_value_map =
      cluster
          .Execute(ClusterHostType::kPrimary,
                   "SELECT id, value FROM SampleTable")
          .MapFrom<SampleRow>()
          .AsContainer<std::unordered_map<std::int32_t, std::string>>();

  static_assert(
      std::is_same_v<const std::unordered_map<std::int32_t, std::string>,
                     decltype(id_value_map)>);
  EXPECT_EQ(id_value_map.size(), data.size());
  for (const auto& [k, v] : data) {
    ASSERT_EQ(id_value_map.at(k), v);
  }
}
/// [uMySQL usage sample - StatementResultSet MapFrom]

UTEST(StatementResultSet, MapFrom) {
  const ClusterWrapper cluster{};

  PrepareSampleTable(*cluster);

  PerformMapFrom(*cluster, {{1, "first"}, {2, "second"}});
}

}  // namespace map_from_sample

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
