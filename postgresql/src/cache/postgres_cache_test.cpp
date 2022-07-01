#include "postgres_cache_test_fwd.hpp"

#include <userver/cache/base_postgres_cache.hpp>

#include <boost/functional/hash.hpp>

#include <userver/components/minimal_server_component_list.hpp>

USERVER_NAMESPACE_BEGIN

// This is a snippet for documentation
/*! [Pg Cache Policy Example] */
namespace example {

struct MyStructure {
  int id;
  std::string bar;
  std::chrono::system_clock::time_point updated;

  int get_id() const { return id; }
};

struct PostgresExamplePolicy {
  // Name of caching policy component.
  //
  // Required: **yes**
  static constexpr std::string_view kName = "my-pg-cache";

  // Object type.
  //
  // Required: **yes**
  using ValueType = MyStructure;

  // Key by which the object must be identified in cache.
  //
  // One of:
  // - A pointer-to-member in the object
  // - A pointer-to-member-function in the object that returns the key
  // - A pointer-to-function that takes the object and returns the key
  // - A lambda that takes the object and returns the key
  //
  // Required: **yes**
  static constexpr auto kKeyMember = &MyStructure::id;

  // Data retrieve query.
  //
  // The query should not contain any clauses after the `from` clause. Either
  // `kQuery` or `GetQuery` static member function must be defined.
  //
  // Required: **yes**
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";

  // Name of the field containing timestamp of an object.
  //
  // To turn off incremental updates, set the value to `nullptr`.
  //
  // Required: **yes**
  static constexpr const char* kUpdatedField = "updated";

  // Type of the field containing timestamp of an object.
  //
  // Specifies whether updated field should be treated as a timestamp
  // with or without timezone in database queries.
  //
  // Required: **yes** if incremantal updates are used.
  using UpdatedFieldType = storages::postgres::TimePointTz;

  // Where clause of the query. Either `kWhere` or `GetWhere` can be defined.
  //
  // Required: no
  static constexpr const char* kWhere = "id > 10";

  // Cache container type.
  //
  // It can be of any map type. The default is `unordered_map`, it is not
  // necessary to declare the DataType alias if you are OK with
  // `unordered_map`.
  // The key type must match the type of kKeyMember.
  //
  // Required: no
  using CacheContainer = std::unordered_map<int, MyStructure>;

  // Cluster host selection flags to use when retrieving data.
  //
  // Default value is storages::postgres::ClusterHostType::kSlave, at least one
  // cluster role must be present in flags.
  //
  // Required: no
  static constexpr auto kClusterHostType =
      storages::postgres::ClusterHostType::kSlave;

  // Whether Get() is expected to return nullptr.
  //
  // Default value is false, Get() will throw an exception instead of
  // returning nullptr.
  //
  // Required: no
  static constexpr bool kMayReturnNull = false;
};

}  // namespace example
/*! [Pg Cache Policy Example] */

namespace components::example {

using USERVER_NAMESPACE::example::MyStructure;
using USERVER_NAMESPACE::example::PostgresExamplePolicy;

struct PostgresExamplePolicy2 {
  using ValueType = MyStructure;
  static constexpr std::string_view kName = "my-pg-cache";
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";
  static constexpr const char* kUpdatedField = "";  // Intentionally left blank
  static constexpr auto kKeyMember = &MyStructure::get_id;
  static constexpr auto kClusterHostType =
      storages::postgres::ClusterHostType::kSlave;
};

static_assert(pg_cache::detail::kHasName<PostgresExamplePolicy>);
static_assert(pg_cache::detail::kHasQuery<PostgresExamplePolicy>);
static_assert(pg_cache::detail::kHasKeyMember<PostgresExamplePolicy>);

static_assert((std::is_same<
               pg_cache::detail::KeyMemberType<PostgresExamplePolicy>, int>{}));
static_assert(
    (std::is_same<pg_cache::detail::KeyMemberType<PostgresExamplePolicy2>,
                  int>{}));

static_assert(pg_cache::detail::ClusterHostType<PostgresExamplePolicy>() ==
              storages::postgres::ClusterHostType::kSlave);
static_assert(pg_cache::detail::ClusterHostType<PostgresExamplePolicy2>() ==
              storages::postgres::ClusterHostType::kSlave);

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
  static constexpr std::string_view kName = "my-pg-cache";
  static constexpr const char* kQuery =
      "select id, bar, revision from test.my_data";
  using CacheContainer = UserSpecificCache;
  static constexpr const char* kUpdatedField = "revision";
  using UpdatedFieldType = int32_t;
  static constexpr auto kKeyMember = &MyStructureWithRevision::get_id;

  // Function to get last known revision/time
  //
  // Optional
  // If one wants to get cache updates not based on updated time, but, for
  // example, based on revision > known_revision, this method should be used.
  static int32_t GetLastKnownUpdated(const UserSpecificCache& container) {
    return container.GetLatestRevision();
  }
};
/*! [Pg Cache Policy Custom Updated Example] */

static_assert(pg_cache::detail::kHasCustomUpdated<PostgresExamplePolicy3>);

/*! [Pg Cache Policy GetQuery Example] */
struct PostgresExamplePolicy4 {
  static constexpr std::string_view kName = "my-pg-cache";

  using ValueType = MyStructure;

  static constexpr auto kKeyMember = &MyStructure::id;

  static std::string GetQuery() {
    return "select id, bar, updated from test.my_data";
  }

  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType =
      storages::postgres::TimePoint;  // no time zone (should be avoided)
};
/*! [Pg Cache Policy GetQuery Example] */

static_assert(pg_cache::detail::kHasGetQuery<PostgresExamplePolicy4>);

/*! [Pg Cache Policy Trivial] */
struct PostgresTrivialPolicy {
  static constexpr std::string_view kName = "my-pg-cache";

  using ValueType = MyStructure;

  static constexpr auto kKeyMember = &MyStructure::id;

  static constexpr const char* kQuery = "SELECT a, b, updated FROM test.data";

  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType = storages::postgres::TimePointTz;
};
/*! [Pg Cache Policy Trivial] */

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
  static constexpr std::string_view kName = "my-pg-cache";

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
  using UpdatedFieldType = storages::postgres::TimePointTz;

  using CacheContainer = std::unordered_map<MyStructureCompoundKey, MyStructure,
                                            MyStructureCompoundKeyHash>;
};
/*! [Pg Cache Policy Compound Primary Key Example] */

static_assert(pg_cache::detail::kHasGetQuery<PostgresExamplePolicy5>);

/*! [Pg Cache Policy Custom Container With Write Notification Example] */
class UserSpecificCacheWithWriteNotification {
 public:
  void insert_or_assign(int, MyStructure&&) {}
  size_t size() const { return 0; }

  void OnWritesDone() {}
};
/*! [Pg Cache Policy Custom Container With Write Notification Example] */

struct PostgresExamplePolicy6 {
  static constexpr std::string_view kName = "my-pg-cache";
  using ValueType = MyStructure;
  static constexpr auto kKeyMember = &MyStructure::id;
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";
  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType = storages::postgres::TimePointTz;
  using CacheContainer = UserSpecificCacheWithWriteNotification;
};

// Instantiation test
using MyCache1 = PostgreCache<PostgresExamplePolicy>;
using MyCache2 = PostgreCache<PostgresExamplePolicy2>;
using MyCache3 = PostgreCache<PostgresExamplePolicy3>;
using MyCache4 = PostgreCache<PostgresExamplePolicy4>;
using MyTrivialCache = PostgreCache<PostgresTrivialPolicy>;
using MyCache5 = PostgreCache<PostgresExamplePolicy5>;
using MyCache6 = PostgreCache<PostgresExamplePolicy6>;

// NB: field access required for actual instantiation
static_assert(MyCache1::kIncrementalUpdates);
static_assert(!MyCache2::kIncrementalUpdates);
static_assert(MyCache3::kIncrementalUpdates);
static_assert(MyCache4::kIncrementalUpdates);
static_assert(MyCache5::kIncrementalUpdates);
static_assert(MyCache6::kIncrementalUpdates);

namespace pg = storages::postgres;
static_assert(MyCache1::kClusterHostTypeFlags == pg::ClusterHostType::kSlave);
static_assert(MyCache2::kClusterHostTypeFlags == pg::ClusterHostType::kSlave);
static_assert(MyCache3::kClusterHostTypeFlags == pg::ClusterHostType::kSlave);
static_assert(MyCache4::kClusterHostTypeFlags == pg::ClusterHostType::kSlave);
static_assert(MyCache5::kClusterHostTypeFlags == pg::ClusterHostType::kSlave);
static_assert(MyCache6::kClusterHostTypeFlags == pg::ClusterHostType::kSlave);

// Update() instantiation test
[[maybe_unused]] void VerifyUpdateCompiles(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  MyCache1{config, context};
  MyCache2{config, context};
  MyCache3{config, context};
  MyCache4{config, context};
  MyCache5{config, context};
  MyCache6{config, context};
}

inline auto SampleOfComponentRegistration() {
  /*! [Pg Cache Trivial Usage] */
  return components::MinimalServerComponentList()
      .Append<components::PostgreCache<example::PostgresTrivialPolicy>>();
  /*! [Pg Cache Trivial Usage] */
}

}  // namespace components::example

USERVER_NAMESPACE_END
