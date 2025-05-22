#include "cache.hpp"

#include <boost/functional/hash.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/projected_set.hpp>

USERVER_NAMESPACE_BEGIN

// This is a snippet for documentation
/*! [Ydb Cache Policy Example] */
namespace example {

struct MyStructure {
  int id = 0;
  std::string bar{};
  std::chrono::system_clock::time_point updated;

  int get_id() const { return id; }
};

struct YdbExamplePolicy {
  // Name of caching policy component.
  //
  // Required: **yes**
  static constexpr std::string_view kName = "ydb-cache";

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
  // Required: **yes** if incremental updates are used.
  using UpdatedFieldType = storages::ydb::DateTime;

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
  // Default value is storages::Ydb::ClusterHostType::kSlave, at least one
  // cluster role must be present in flags.
  //
  // Required: no
  static constexpr auto kClusterHostType =
      ydb::ClusterHostType::kPrimary;

  // Whether Get() is expected to return nullptr.
  //
  // Default value is false, Get() will throw an exception instead of
  // returning nullptr.
  //
  // Required: no
  static constexpr bool kMayReturnNull = false;
};

}  // namespace example
/*! [Ydb Cache Policy Example] */

namespace components::example {

using USERVER_NAMESPACE::example::MyStructure;
using USERVER_NAMESPACE::example::YdbExamplePolicy;

struct YdbExamplePolicy2 {
  using ValueType = MyStructure;
  static constexpr std::string_view kName = "ydb-cache";
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";
  static constexpr const char* kUpdatedField = "";  // Intentionally left blank
  static constexpr auto kKeyMember = &MyStructure::get_id;
  static constexpr auto kClusterHostType =
      storages::ydb::ClusterHostType::kPrimary;
};

static_assert(ydb_cache::detail::kHasName<YdbExamplePolicy>);
static_assert(ydb_cache::detail::kHasName<YdbExamplePolicy>);
static_assert(ydb_cache::detail::kHasName<YdbExamplePolicy>);

static_assert((std::is_same<
               ydb_cache::detail::KeyMemberType<YdbExamplePolicy>, int>{}));
static_assert(
    (std::is_same<ydb_cache::detail::KeyMemberType<YdbExamplePolicy2>,
                  int>{}));

static_assert(ydb_cache::detail::ClusterHostType<YdbExamplePolicy>() ==
              storages::ydb::ClusterHostType::kPrimary);
static_assert(ydb_cache::detail::ClusterHostType<YdbExamplePolicy2>() ==
              storages::ydb::ClusterHostType::kPrimary);

// Example of custom updated in cache
/*! [Ydb Cache Policy Custom Updated Example] */
struct MyStructureWithRevision {
  int id = 0;
  std::string bar{};
  std::chrono::system_clock::time_point updated;
  int32_t revision = 0;

  int get_id() const { return id; }
};

class UserSpecificCache {
 public:
  void insert_or_assign(int, MyStructureWithRevision&& item) {
    latest_revision_ = std::max(latest_revision_, item.revision);
  }
  // todo ivan check
  void insert_or_assign(int, MyStructureWithRevision& item) {
    latest_revision_ = std::max(latest_revision_, item.revision);
  }
  static size_t size() { return 0; }

  int GetLatestRevision() const { return latest_revision_; }

 private:
  int latest_revision_ = 0;
};

struct YdbExamplePolicy3 {
  using ValueType = MyStructureWithRevision;
  static constexpr std::string_view kName = "ydb-cache";
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
/*! [Ydb Cache Policy Custom Updated Example] */

static_assert(ydb_cache::detail::kHasCustomUpdated<YdbExamplePolicy3>);

/*! [Ydb Cache Policy GetQuery Example] */
struct YdbExamplePolicy4 {
  static constexpr std::string_view kName = "ydb-cache";

  using ValueType = MyStructure;

  static constexpr auto kKeyMember = &MyStructure::id;

  static std::string GetQuery() {
    return "select id, bar, updated from test.my_data";
  }

  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType =
      storages::ydb::DateTime;  // no time zone (should be avoided)
};
/*! [Ydb Cache Policy GetQuery Example] */

static_assert(ydb_cache::detail::kHasGetQuery<YdbExamplePolicy4>);

/*! [Ydb Cache Policy Trivial] */
struct YdbTrivialPolicy {
  static constexpr std::string_view kName = "ydb-cache";

  using ValueType = MyStructure;

  static constexpr auto kKeyMember = &MyStructure::id;

  static constexpr const char* kQuery = "SELECT a, b, updated FROM test.data";

  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType = storages::ydb::DateTime;
};
/*! [Ydb Cache Policy Trivial] */

/*! [Ydb Cache Policy Compound Primary Key Example] */
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

struct YdbExamplePolicy5 {
  static constexpr std::string_view kName = "ydb-cache";

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
  using UpdatedFieldType = storages::ydb::DateTime;

  using CacheContainer = std::unordered_map<MyStructureCompoundKey, MyStructure,
                                            MyStructureCompoundKeyHash>;
};
/*! [Ydb Cache Policy Compound Primary Key Example] */

static_assert(ydb_cache::detail::kHasGetQuery<YdbExamplePolicy5>);

/*! [Ydb Cache Policy Custom Container With Write Notification Example] */
class UserSpecificCacheWithWriteNotification {
 public:
  void insert_or_assign(int, MyStructure&&) {}
  void insert_or_assign(int, MyStructure&) {}
  static size_t size() { return 0; }

  void OnWritesDone() {}
};
/*! [Ydb Cache Policy Custom Container With Write Notification Example] */

// Tests a container with OnWritesDone
struct YdbExamplePolicy6 {
  static constexpr std::string_view kName = "ydb-cache";
  using ValueType = MyStructure;
  static constexpr auto kKeyMember = &MyStructure::id;
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";
  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType = storages::ydb::DateTime;
  using CacheContainer = UserSpecificCacheWithWriteNotification;
};

// Tests ProjectedUnorderedSet as container
struct YdbExamplePolicy7 {
  static constexpr std::string_view kName = "ydb-cache";
  using ValueType = MyStructure;
  static constexpr auto kKeyMember = &MyStructure::id;
  static constexpr const char* kQuery =
      "select id, bar, updated from test.my_data";
  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType = storages::ydb::DateTime;
  using CacheContainer = utils::ProjectedUnorderedSet<ValueType, kKeyMember>;
};

// Instantiation test

using MyCache1 = YdbCache<YdbExamplePolicy>;
using MyCache2 = YdbCache<YdbExamplePolicy2>;
using MyCache3 = YdbCache<YdbExamplePolicy3>;
using MyCache4 = YdbCache<YdbExamplePolicy4>;
using MyTrivialCache = YdbCache<YdbTrivialPolicy>;
using MyCache5 = YdbCache<YdbExamplePolicy5>;
using MyCache6 = YdbCache<YdbExamplePolicy6>;
using MyCache7 = YdbCache<YdbExamplePolicy7>;

// NB: field access required for actual instantiation
static_assert(MyCache1::kIncrementalUpdates);
static_assert(!MyCache2::kIncrementalUpdates);
static_assert(MyCache3::kIncrementalUpdates);
static_assert(MyCache4::kIncrementalUpdates);
static_assert(MyCache5::kIncrementalUpdates);
static_assert(MyCache6::kIncrementalUpdates);
static_assert(MyCache7::kIncrementalUpdates);

namespace ydb = storages::ydb;
static_assert(MyCache1::kClusterHostTypeFlags == ydb::ClusterHostType::kPrimary);
static_assert(MyCache2::kClusterHostTypeFlags == ydb::ClusterHostType::kPrimary);
static_assert(MyCache3::kClusterHostTypeFlags == ydb::ClusterHostType::kPrimary);
static_assert(MyCache4::kClusterHostTypeFlags == ydb::ClusterHostType::kPrimary);
static_assert(MyCache5::kClusterHostTypeFlags == ydb::ClusterHostType::kPrimary);
static_assert(MyCache6::kClusterHostTypeFlags == ydb::ClusterHostType::kPrimary);
static_assert(MyCache7::kClusterHostTypeFlags == ydb::ClusterHostType::kPrimary);

// Update() instantiation test
[[maybe_unused]] void VerifyUpdateCompiles(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  MyCache1 cache1{config, context};
  MyCache2 cache2{config, context};
  MyCache3 cache3{config, context};
  MyCache4 cache4{config, context};
  MyTrivialCache my_trivial_cache{config, context};
  MyCache5 cache5{config, context};
  MyCache6 cache6{config, context};
  MyCache7 cache7{config, context};
}

inline auto SampleOfComponentRegistration() {
  /*! [Ydb Cache Trivial Usage] */
  return components::MinimalServerComponentList()
      .Append<components::YdbCache<example::YdbTrivialPolicy>>();
  /*! [Ydb Cache Trivial Usage] */
}

}  // namespace components::example

USERVER_NAMESPACE_END