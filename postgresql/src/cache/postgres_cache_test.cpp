#include <cache/base_postgres_cache.hpp>

namespace components::test {

namespace pg = storages::postgres;

// This is a snippet for documentation
/*! [Pg Cache Policy Example] */
struct MyStructure {
  int id;
  std::string bar;
  std::chrono::system_clock::time_point updated;

  int get_id() const { return id; }
};

struct PostgresExamplePolicy {
  // Name of caching policy component.
  // Mandatory
  static constexpr const char* kName = "my-pg-cache";

  // Object type
  // Mandatory
  using ValueType = MyStructure;

  // A member by which the object must be identified in cache.
  // Mandatory
  // Pointer-to-member in the object.
  // It can be either a pointer to data member or a pointer to function member,
  // that accepts no arguments and returns the key.
  static constexpr auto kKeyMember = &MyStructure::id;

  // Data retrieve query.
  // Mandatory
  // The query should not contain any clauses after the `from` clause
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";

  // Name of the field containing timestamp of an object
  // Mandatory
  // To turn off incremental updates, set the value to `nullptr`
  static constexpr const char* kUpdatedField = "updated";

  // Cache container type.
  // Optional
  // It can be of any map type. The default is unordered_map, it is not
  // necessary to declare the DataType alias if you are OK with unordered_map.
  // The key type must match the type of kKeyMember.
  using DataType = std::unordered_map<int, MyStructure>;

  // @brief Cluster host to use when retrieving data
  // Optional
  // Default value is pg::ClusterHostType::kSlave, pg::ClusterHostType::kAny is
  // prohibited
  static constexpr auto kClusterHostType = pg::ClusterHostType::kSlave;
};
/*! [Pg Cache Policy Example] */

struct PostgresExamplePolicy2 {
  using ValueType = MyStructure;
  static constexpr const char* kName = "my-pg-cache";
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";
  static constexpr const char* kUpdatedField = "";  // Intentionally left blank
  static constexpr auto kKeyMember = &MyStructure::get_id;
  static constexpr auto kClusterHostType = pg::ClusterHostType::kSyncSlave;
};

static_assert(pg_cache::detail::kHasName<PostgresExamplePolicy>, "");
static_assert(pg_cache::detail::kHasQuery<PostgresExamplePolicy>, "");
static_assert(pg_cache::detail::kHasKeyMember<PostgresExamplePolicy>, "");

static_assert(
    (std::is_same<pg_cache::detail::KeyMemberType<PostgresExamplePolicy>,
                  int>{}),
    "");
static_assert(
    (std::is_same<pg_cache::detail::KeyMemberType<PostgresExamplePolicy2>,
                  int>{}),
    "");

static_assert(pg_cache::detail::kPostgresClusterType<PostgresExamplePolicy> ==
                  pg::ClusterHostType::kSlave,
              "");
static_assert(pg_cache::detail::kPostgresClusterType<PostgresExamplePolicy2> ==
                  pg::ClusterHostType::kSyncSlave,
              "");

// Instantiation test
using MyCache1 = PostgreCache<PostgresExamplePolicy>;
using MyCache2 = PostgreCache<PostgresExamplePolicy2>;

static_assert(MyCache1::kIncrementalUpdates, "");
static_assert(!MyCache2::kIncrementalUpdates, "");

static_assert(MyCache1::kClusterHostType == pg::ClusterHostType::kSlave, "");
static_assert(MyCache2::kClusterHostType == pg::ClusterHostType::kSyncSlave,
              "");

}  // namespace components::test
