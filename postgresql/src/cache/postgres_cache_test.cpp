#include <cache/base_postgres_cache.hpp>

#include <boost/functional/hash.hpp>

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
  /// @brief Name of caching policy component
  ///
  /// Mandatory
  static constexpr const char* kName = "my-pg-cache";

  /// @brief Object type
  ///
  /// Mandatory
  using ValueType = MyStructure;

  /// @brief Key by which the object must be identified in cache
  ///
  /// Mandatory
  /// One of:
  /// - A pointer-to-member in the object
  /// - A pointer-to-member-function in the object that returns the key
  /// - A pointer-to-function that takes the object and returns the key
  /// - A lambda that takes the object and returns the key
  static constexpr auto kKeyMember = &MyStructure::id;

  /// @brief Data retrieve query
  ///
  /// Mandatory
  /// The query should not contain any clauses after the `from` clause. Either
  /// `kQuery` or `GetQuery` static member function must be defined.
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";

  /// @brief Name of the field containing timestamp of an object
  ///
  /// Mandatory
  /// To turn off incremental updates, set the value to `nullptr`.
  static constexpr const char* kUpdatedField = "updated";

  /// @brief Where clause of the query
  ///
  /// Optional
  /// Either `kWhere` or `GetWhere` can be defined.
  static constexpr const char* kWhere = "id > 10";

  /// @brief Cache container type
  ///
  /// Optional
  /// It can be of any map type. The default is `unordered_map`, it is not
  /// necessary to declare the DataType alias if you are OK with
  /// `unordered_map`.
  /// The key type must match the type of kKeyMember.
  using CacheContainer = std::unordered_map<int, MyStructure>;

  /// @brief Cluster host selection flags to use when retrieving data
  ///
  /// Optional
  /// Default value is pg::ClusterHostType::kSlave, at least one cluster role
  /// must be present in flags.
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
  static constexpr auto kClusterHostType = pg::ClusterHostType::kSlave;
};

static_assert(pg_cache::detail::kHasName<PostgresExamplePolicy>);
static_assert(pg_cache::detail::kHasQuery<PostgresExamplePolicy>);
static_assert(pg_cache::detail::kHasKeyMember<PostgresExamplePolicy>);

static_assert((std::is_same<
               pg_cache::detail::KeyMemberType<PostgresExamplePolicy>, int>{}));
static_assert(
    (std::is_same<pg_cache::detail::KeyMemberType<PostgresExamplePolicy2>,
                  int>{}));

static_assert(
    pg_cache::detail::kPostgresClusterHostTypeFlags<PostgresExamplePolicy> ==
    pg::ClusterHostType::kSlave);
static_assert(
    pg_cache::detail::kPostgresClusterHostTypeFlags<PostgresExamplePolicy2> ==
    pg::ClusterHostType::kSlave);

// Example of custom updated in cache
/*! [Pg Cache Policy Custom Updated Example] */
struct MyStructureWithRevision {
  int id;
  std::string bar;
  std::chrono::system_clock::time_point updated;
  int32_t revision;

  int get_id() const { return id; }
};

class UserSpecificCache {
 public:
  void insert_or_assign(int, MyStructureWithRevision&& item) {
    latest_revision_ = std::max(latest_revision_, item.revision);
  }
  size_t size() const { return 0; }

  int GetLatestRevision() const { return latest_revision_; }

 private:
  int latest_revision_ = 0;
};

struct PostgresExamplePolicy3 {
  using ValueType = MyStructureWithRevision;
  static constexpr const char* kName = "my-pg-cache";
  static constexpr const char* kQuery =
      "select id, bar, revision from test.my_data";
  using CacheContainer = UserSpecificCache;
  static constexpr const char* kUpdatedField = "revision";
  static constexpr auto kKeyMember = &MyStructureWithRevision::get_id;

  /// @brief Function to get last known revision/time
  ///
  /// Optional
  /// If one wants to get cache updates not based on updated time, but, for
  /// example, based on revision > known_revision, this method should be used.
  static int32_t GetLastKnownUpdated(const UserSpecificCache& container) {
    return container.GetLatestRevision();
  }
};
/*! [Pg Cache Policy Custom Updated Example] */

static_assert(pg_cache::detail::kHasCustomUpdated<PostgresExamplePolicy3>);

/*! [Pg Cache Policy GetQuery Example] */
struct PostgresExamplePolicy4 {
  static constexpr const char* kName = "my-pg-cache";

  using ValueType = MyStructure;

  static constexpr auto kKeyMember = &MyStructure::id;

  static std::string GetQuery() {
    return "select id, bar, updated from test.my_data";
  }

  static constexpr const char* kUpdatedField = "updated";
};
/*! [Pg Cache Policy GetQuery Example] */

static_assert(pg_cache::detail::kHasGetQuery<PostgresExamplePolicy4>);

/*! [Pg Cache Policy Compound Primary Key Example] */
struct MyStructureCompoundKey {
  int id;
  std::string bar;

  bool operator==(const MyStructureCompoundKey& other) const {
    return id == other.id && bar == other.bar;
  }
};

// Alternatively, specialize std::hash
struct MyStructureCompoundKeyHash {
  size_t operator()(const MyStructureCompoundKey& key) const {
    size_t seed = 0;
    boost::hash_combine(seed, key.id);
    boost::hash_combine(seed, key.bar);
    return seed;
  }
};

struct PostgresExamplePolicy5 {
  static constexpr const char* kName = "my-pg-cache";

  using ValueType = MyStructure;

  // maybe_unused is required due to a Clang bug
  [[maybe_unused]] static constexpr auto kKeyMember =
      [](const MyStructure& my_structure) {
        return MyStructureCompoundKey{my_structure.id, my_structure.bar};
      };

  static std::string GetQuery() {
    return "select id, bar, updated from test.my_data";
  }

  static constexpr const char* kUpdatedField = "updated";

  using CacheContainer = std::unordered_map<MyStructureCompoundKey, MyStructure,
                                            MyStructureCompoundKeyHash>;
};
/*! [Pg Cache Policy Compound Primary Key Example] */

static_assert(pg_cache::detail::kHasGetQuery<PostgresExamplePolicy5>);

// Instantiation test
using MyCache1 = PostgreCache<PostgresExamplePolicy>;
using MyCache2 = PostgreCache<PostgresExamplePolicy2>;
using MyCache3 = PostgreCache<PostgresExamplePolicy3>;
using MyCache4 = PostgreCache<PostgresExamplePolicy4>;
using MyCache5 = PostgreCache<PostgresExamplePolicy5>;

static_assert(MyCache1::kIncrementalUpdates);
static_assert(!MyCache2::kIncrementalUpdates);

static_assert(MyCache1::kClusterHostTypeFlags == pg::ClusterHostType::kSlave);
static_assert(MyCache2::kClusterHostTypeFlags == pg::ClusterHostType::kSlave);

}  // namespace components::test
