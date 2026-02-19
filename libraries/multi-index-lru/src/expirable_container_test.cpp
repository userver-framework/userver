#include <userver/multi-index-lru/expirable_container.hpp>
#include <userver/utils/async.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>

#include <string>
#include <mutex>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
class ExpirableUsersTest : public ::testing::Test {
protected:
    void SetUp() override {}

    struct IdTag {};
    struct EmailTag {};
    struct NameTag {};

    struct User {
        int id;
        std::string email;
        std::string name;

        bool operator==(const User& other) const {
            return id == other.id && email == other.email && name == other.name;
        }
    };
    
    using UserCacheExpirable = multi_index_lru::ExpirableContainer<
        User,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<IdTag>,
                boost::multi_index::member<User, int, &User::id>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<EmailTag>,
                boost::multi_index::member<User, std::string, &User::email>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NameTag>,
                boost::multi_index::member<User, std::string, &User::name>>>>;
};

UTEST_F(ExpirableUsersTest, BasicOperations) {
    UserCacheExpirable cache(3, std::chrono::seconds(10));  // capacity=3, TTL=10s

    // Test insertion
    EXPECT_TRUE(cache.insert(User{1, "alice@test.com", "Alice"}));
    EXPECT_TRUE(cache.insert(User{2, "bob@test.com", "Bob"}));
    EXPECT_TRUE(cache.insert(User{3, "charlie@test.com", "Charlie"}));

    EXPECT_EQ(cache.size(), 3);
    EXPECT_EQ(cache.capacity(), 3);
    EXPECT_FALSE(cache.empty());

    // Test get by id (unique index) – returns vector
    auto alice_vec = cache.get<IdTag>(1);
    ASSERT_EQ(alice_vec.size(), 1);
    EXPECT_EQ(alice_vec[0].name, "Alice");

    // Test get by email (unique index)
    auto bob_vec = cache.get<EmailTag>("bob@test.com");
    ASSERT_EQ(bob_vec.size(), 1);
    EXPECT_EQ(bob_vec[0].id, 2);

    // Test get by name (non‑unique index) – returns all with that name
    auto charlie_vec = cache.get<NameTag>("Charlie");
    ASSERT_EQ(charlie_vec.size(), 1);
    EXPECT_EQ(charlie_vec[0].email, "charlie@test.com");
}

UTEST_F(ExpirableUsersTest, LRUEviction) {
    UserCacheExpirable cache(3, std::chrono::seconds(10));

    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    cache.insert(User{3, "charlie@test.com", "Charlie"});

    // Access Alice and Charlie to make them recently used (contains updates timestamp)
    EXPECT_TRUE(cache.contains<IdTag>(1));
    EXPECT_TRUE(cache.contains<IdTag>(3));

    // Add fourth element - Bob should be evicted (LRU)
    cache.insert(User{4, "david@test.com", "David"});

    EXPECT_FALSE(cache.contains<IdTag>(2));  // Bob evicted (LRU)
    EXPECT_TRUE(cache.contains<IdTag>(1));   // Alice remains
    EXPECT_TRUE(cache.contains<IdTag>(3));   // Charlie remains
    EXPECT_TRUE(cache.contains<IdTag>(4));   // David added
    EXPECT_EQ(cache.size(), 3);
}

UTEST_F(ExpirableUsersTest, TTLExpiration) {
    using namespace std::chrono_literals;
    
    UserCacheExpirable cache(100, 100ms);  // Very short TTL for testing

    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    
    // Items should still exist
    EXPECT_TRUE(cache.contains<IdTag>(1));
    EXPECT_TRUE(cache.contains<IdTag>(2));
    EXPECT_EQ(cache.size(), 2);
    
    // Wait for TTL to expire
    userver::engine::SleepFor(150ms);
    
    EXPECT_FALSE(cache.contains<IdTag>(1));
    EXPECT_FALSE(cache.contains<IdTag>(2));
    EXPECT_EQ(cache.size(), 0);
}

UTEST_F(ExpirableUsersTest, TTLRefreshOnAccess) {
    using namespace std::chrono_literals;
    
    UserCacheExpirable cache(100, 190ms);

    cache.insert(User{1, "alice@test.com", "Alice"});
    
    // Wait a bit but not enough to expire
    userver::engine::SleepFor(99ms);
    // Access via contains should refresh TTL
    EXPECT_TRUE(cache.contains<IdTag>(1));
    
    // Wait again - should still be alive due to refresh
    userver::engine::SleepFor(99ms);
    EXPECT_TRUE(cache.contains<IdTag>(1));
    
    // Wait for full TTL from last access
    userver::engine::SleepFor(200ms);
    EXPECT_FALSE(cache.contains<IdTag>(1));
}

UTEST_F(ExpirableUsersTest, EraseOperations) {
    UserCacheExpirable cache(3, std::chrono::seconds(10));
    
    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    
    EXPECT_TRUE(cache.erase<IdTag>(1));
    EXPECT_FALSE(cache.contains<IdTag>(1));
    EXPECT_TRUE(cache.contains<IdTag>(2));
    EXPECT_EQ(cache.size(), 1);
    
    EXPECT_FALSE(cache.erase<IdTag>(999));  // Non-existent
    EXPECT_EQ(cache.size(), 1);
}

UTEST_F(ExpirableUsersTest, SetCapacity) {
    UserCacheExpirable cache(5, std::chrono::seconds(10));
    
    // Fill cache
    for (int i = 1; i <= 5; ++i) {
        cache.insert(User{i, std::to_string(i) + "@test.com", "User" + std::to_string(i)});
    }
    EXPECT_EQ(cache.size(), 5);
    EXPECT_EQ(cache.capacity(), 5);
    
    // Reduce capacity - should evict LRU items
    cache.set_capacity(3);
    EXPECT_EQ(cache.capacity(), 3);
    
    // Size should be <= new capacity
    EXPECT_LE(cache.size(), 3);
}

UTEST_F(ExpirableUsersTest, Clear) {
    UserCacheExpirable cache(5, std::chrono::seconds(10));
    
    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    
    EXPECT_EQ(cache.size(), 2);
    EXPECT_FALSE(cache.empty());
    
    cache.clear();
    
    EXPECT_EQ(cache.size(), 0);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.contains<IdTag>(1));
    EXPECT_FALSE(cache.contains<IdTag>(2));
}

UTEST_F(ExpirableUsersTest, ThreadSafetyBasic) {
    // Container is not thread-safe; external synchronization required.
    UserCacheExpirable cache(100, std::chrono::seconds(10));
    engine::Mutex mutex;
    
    constexpr int kCoroutines = 4;
    constexpr int kIterations = 100;
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kCoroutines);
    
    for (int t = 0; t < kCoroutines; ++t) {
        tasks.push_back(utils::Async("using cache", [&cache, &mutex, t]() {
            for (int i = 0; i < kIterations; ++i) {
                int id = t * kIterations + i;
                
                {
                    std::lock_guard<engine::Mutex> lock(mutex);
                    cache.insert(User{id, std::to_string(id) + "@test.com", "User" + std::to_string(id)});
                }
                
                if (id % 3 == 0) {
                    std::lock_guard<engine::Mutex> lock(mutex);
                    // Use contains to check existence and update timestamp
                    cache.contains<IdTag>(id);
                }
                
                if (id % 5 == 0) {
                    std::lock_guard<engine::Mutex> lock(mutex);
                    cache.erase<IdTag>(id - 1);
                }
            }
        }));
    }
    
    for (auto& task : tasks) {
        task.Get();
    }
    
    std::lock_guard<engine::Mutex> lock(mutex);
    EXPECT_LE(cache.size(), 100);
}

}  // namespace

USERVER_NAMESPACE_END