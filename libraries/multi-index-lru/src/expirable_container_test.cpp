#include <userver/multi-index-lru/container.hpp>
#include <userver/multi-index-lru/expirable_container.hpp>
#include <userver/utils/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task_with_result.hpp>

#include <string>

#include <gtest/gtest.h>
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

TEST_F(ExpirableUsersTest, BasicOperations) {
    userver::engine::RunStandalone([&] {
    UserCacheExpirable cache(3, std::chrono::seconds(10));  // capacity=3, TTL=10s

    // Test insertion
    EXPECT_TRUE(cache.insert(User{1, "alice@test.com", "Alice"}));
    EXPECT_TRUE(cache.insert(User{2, "bob@test.com", "Bob"}));
    EXPECT_TRUE(cache.insert(User{3, "charlie@test.com", "Charlie"}));

    EXPECT_EQ(cache.size(), 3);
    EXPECT_EQ(cache.capacity(), 3);
    EXPECT_FALSE(cache.empty());

    // Test find by id
    auto by_id = cache.find<IdTag>(1);
    ASSERT_NE(by_id, cache.end<IdTag>());
    EXPECT_EQ(by_id->name, "Alice");

    // Test find by email
    auto by_email = cache.find<EmailTag>("bob@test.com");
    ASSERT_NE(by_email, cache.end<EmailTag>());
    EXPECT_EQ(by_email->id, 2);

    // Test find by name
    auto by_name = cache.find<NameTag>("Charlie");
    ASSERT_NE(by_name, cache.end<NameTag>());
    EXPECT_EQ(by_name->email, "charlie@test.com");
    });
}

TEST_F(ExpirableUsersTest, LRUEviction) {
    userver::engine::RunStandalone([&] {
    UserCacheExpirable cache(3, std::chrono::seconds(10));

    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    cache.insert(User{3, "charlie@test.com", "Charlie"});

    // Access Alice and Charlie to make them recently used
    cache.find<IdTag>(1);
    cache.find<IdTag>(3);

    // Add fourth element - Bob should be evicted (LRU)
    cache.insert(User{4, "david@test.com", "David"});

    EXPECT_FALSE(cache.contains<IdTag>(2));  // Bob evicted (LRU)
    EXPECT_TRUE(cache.contains<IdTag>(1));   // Alice remains
    EXPECT_TRUE(cache.contains<IdTag>(3));   // Charlie remains
    EXPECT_TRUE(cache.contains<IdTag>(4));   // David added
    EXPECT_EQ(cache.size(), 3);
    });
}

TEST_F(ExpirableUsersTest, TTLExpiration) {
    userver::engine::RunStandalone([&] {
    using namespace std::chrono_literals;
    
    UserCacheExpirable cache(100, 100ms);  // Very short TTL for testing
    
    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    
    // Items should still exist
    EXPECT_TRUE(cache.contains<IdTag>(1));
    EXPECT_TRUE(cache.contains<IdTag>(2));
    EXPECT_EQ(cache.size(), 2);
    
    // Wait for TTL to expire
    std::this_thread::sleep_for(150ms);
    
    EXPECT_FALSE(cache.contains<IdTag>(1));
    EXPECT_FALSE(cache.contains<IdTag>(2));
    EXPECT_EQ(cache.size(), 0);
    });
}

TEST_F(ExpirableUsersTest, TTLRefreshOnAccess) {
    userver::engine::RunStandalone([&] {
    using namespace std::chrono_literals;
    
    UserCacheExpirable cache(100, 190ms);
    
    cache.insert(User{1, "alice@test.com", "Alice"});
    
    // Wait a bit but not enough to expire
    std::this_thread::sleep_for(100ms);
    
    // Access should refresh TTL
    EXPECT_TRUE(cache.contains<IdTag>(1));
    
    // Wait again - should still be alive due to refresh
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(cache.contains<IdTag>(1));
    
    // Wait for full TTL from last access
    std::this_thread::sleep_for(200ms);
    EXPECT_FALSE(cache.contains<IdTag>(1));
    });
}

TEST_F(ExpirableUsersTest, EraseOperations) {
    userver::engine::RunStandalone([&] {
    UserCacheExpirable cache(3, std::chrono::seconds(10));
    
    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    
    EXPECT_TRUE(cache.erase<IdTag>(1));
    EXPECT_FALSE(cache.contains<IdTag>(1));
    EXPECT_TRUE(cache.contains<IdTag>(2));
    EXPECT_EQ(cache.size(), 1);
    
    EXPECT_FALSE(cache.erase<IdTag>(999));  // Non-existent
    EXPECT_EQ(cache.size(), 1);
    });
}

TEST_F(ExpirableUsersTest, SetCapacity) {
    userver::engine::RunStandalone([&] {
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
    });
}

TEST_F(ExpirableUsersTest, Clear) {
    userver::engine::RunStandalone([&] {
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
    });
}

TEST_F(ExpirableUsersTest, ThreadSafetyBasic) {
    userver::engine::RunStandalone([&] {
    UserCacheExpirable cache(100, std::chrono::seconds(10));
    
    constexpr int kCoroutines = 4;
    constexpr int kIterations = 100;
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kCoroutines);
    
    for (int t = 0; t < kCoroutines; ++t) {
        tasks.push_back(utils::Async("using cache", [&cache, t]() {
            for (int i = 0; i < kIterations; ++i) {
                int id = t * kIterations + i;
                
                cache.insert(User{id, std::to_string(id) + "@test.com", "User" + std::to_string(id)});
                
                if (id % 3 == 0) {
                    cache.find<IdTag>(id);
                    cache.contains<IdTag>(id);
                }
                
                if (id % 5 == 0) {
                    cache.erase<IdTag>(id - 1);
                }
            }
        }));
    }
    
    for (auto& task : tasks) {
        task.Get();
    }
    
    EXPECT_LE(cache.size(), 100);
    });
}
}  // namespace

USERVER_NAMESPACE_END