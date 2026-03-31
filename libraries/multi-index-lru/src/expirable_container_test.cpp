#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/multi-index-lru/expirable_container.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

#include <mutex>
#include <string>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

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

    // Test find by id (unique index)
    auto alice_it = cache.find<IdTag>(1);
    EXPECT_NE(alice_it, cache.end<IdTag>());
    EXPECT_EQ(alice_it->name, "Alice");

    // Test find by email (unique index)
    auto bob_it = cache.find<EmailTag>("bob@test.com");
    EXPECT_NE(bob_it, cache.end<EmailTag>());
    EXPECT_EQ(bob_it->id, 2);

    // Test find by name (non-unique index) - returns first match
    auto charlie_it = cache.find<NameTag>("Charlie");
    EXPECT_NE(charlie_it, cache.end<NameTag>());
    EXPECT_EQ(charlie_it->email, "charlie@test.com");
}

UTEST_F(ExpirableUsersTest, FindNoUpdate) {
    UserCacheExpirable cache(3, std::chrono::seconds(10));

    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    cache.insert(User{3, "charlie@test.com", "Charlie"});

    // Both finds should succeed
    EXPECT_NE(cache.find<IdTag>(1), cache.end<IdTag>());
    EXPECT_NE(cache.find_no_update<IdTag>(1), cache.end<IdTag>());
}

UTEST_F(ExpirableUsersTest, LRUEviction) {
    UserCacheExpirable cache(3, std::chrono::seconds(10));

    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});
    cache.insert(User{3, "charlie@test.com", "Charlie"});

    // Access Alice and Charlie to make them recently used
    EXPECT_NE(cache.find<IdTag>(1), cache.end<IdTag>());
    EXPECT_NE(cache.find<IdTag>(3), cache.end<IdTag>());

    // Add fourth element - Bob should be evicted (LRU)
    cache.insert(User{4, "david@test.com", "David"});

    EXPECT_EQ(cache.find<IdTag>(2), cache.end<IdTag>());  // Bob evicted (LRU)
    EXPECT_NE(cache.find<IdTag>(1), cache.end<IdTag>());  // Alice remains
    EXPECT_NE(cache.find<IdTag>(3), cache.end<IdTag>());  // Charlie remains
    EXPECT_NE(cache.find<IdTag>(4), cache.end<IdTag>());  // David added
    EXPECT_EQ(cache.size(), 3);
}

UTEST_F(ExpirableUsersTest, TTLExpiration) {
    using namespace std::chrono_literals;

    UserCacheExpirable cache(100, 100ms);  // Very short TTL for testing

    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});

    // Items should still exist
    EXPECT_NE(cache.find<IdTag>(1), cache.end<IdTag>());
    EXPECT_NE(cache.find<IdTag>(2), cache.end<IdTag>());
    EXPECT_EQ(cache.size(), 2);

    // Wait for TTL to expire
    engine::SleepFor(150ms);

    EXPECT_EQ(cache.find<IdTag>(1), cache.end<IdTag>());
    EXPECT_EQ(cache.find<IdTag>(2), cache.end<IdTag>());
    EXPECT_EQ(cache.size(), 0);
}

UTEST_F(ExpirableUsersTest, TTLRefreshOnAccess) {
    using namespace std::chrono_literals;

    UserCacheExpirable cache(100, 190ms);

    cache.insert(User{1, "alice@test.com", "Alice"});

    // Wait a bit but not enough to expire
    engine::SleepFor(99ms);

    // Access via find should refresh TTL
    EXPECT_NE(cache.find<IdTag>(1), cache.end<IdTag>());

    // Wait again - should still be alive due to refresh
    engine::SleepFor(99ms);
    EXPECT_NE(cache.find<IdTag>(1), cache.end<IdTag>());

    // Wait for full TTL from last access
    engine::SleepFor(200ms);
    EXPECT_EQ(cache.find<IdTag>(1), cache.end<IdTag>());
}

UTEST_F(ExpirableUsersTest, EqualRangeOperations) {
    using namespace std::chrono_literals;

    UserCacheExpirable cache(10, 1h);  // Long TTL to avoid expiration

    // Insert multiple users with the same name
    cache.insert(User{1, "john1@test.com", "John"});
    cache.insert(User{2, "john2@test.com", "John"});
    cache.insert(User{3, "john3@test.com", "John"});
    cache.insert(User{4, "alice@test.com", "Alice"});

    // Test equal_range for non-unique index
    auto [begin, end] = cache.equal_range<NameTag>("John");

    // Count matches
    int count = 0;
    for (auto it = begin; it != end; ++it) {
        ++count;
        EXPECT_EQ(it->name, "John");
    }
    EXPECT_EQ(count, 3);

    // Test equal_range for non-existent key
    auto [begin_empty, end_empty] = cache.equal_range<NameTag>("NonExistent");
    EXPECT_EQ(begin_empty, end_empty);
}

UTEST_F(ExpirableUsersTest, EqualRangeNoUpdate) {
    using namespace std::chrono_literals;

    UserCacheExpirable cache(10, 1h);

    cache.insert(User{1, "john1@test.com", "John"});
    cache.insert(User{2, "john2@test.com", "John"});

    // equal_range_no_update should work and find all matches
    auto [begin, end] = cache.equal_range_no_update<NameTag>("John");

    int count = 0;
    for (auto it = begin; it != end; ++it) {
        ++count;
        EXPECT_TRUE(it->id == 1 || it->id == 2);
    }
    EXPECT_EQ(count, 2);
}

UTEST_F(ExpirableUsersTest, EraseOperations) {
    UserCacheExpirable cache(3, std::chrono::seconds(10));

    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});

    EXPECT_TRUE(cache.erase<IdTag>(1));
    EXPECT_EQ(cache.find<IdTag>(1), cache.end<IdTag>());
    EXPECT_NE(cache.find<IdTag>(2), cache.end<IdTag>());
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
    EXPECT_EQ(cache.find<IdTag>(1), cache.end<IdTag>());
    EXPECT_EQ(cache.find<IdTag>(2), cache.end<IdTag>());
}

UTEST_F(ExpirableUsersTest, CleanupExpired) {
    using namespace std::chrono_literals;

    UserCacheExpirable cache(5, 100ms);

    cache.insert(User{1, "alice@test.com", "Alice"});
    cache.insert(User{2, "bob@test.com", "Bob"});

    // Wait for TTL to expire
    engine::SleepFor(150ms);

    // cleanup_expired should remove expired items
    cache.cleanup_expired();

    EXPECT_EQ(cache.size(), 0);
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
                    const std::lock_guard lock{mutex};
                    cache.insert(User{id, std::to_string(id) + "@test.com", "User" + std::to_string(id)});
                }

                if (id % 3 == 0) {
                    const std::lock_guard lock{mutex};
                    // Use find to check existence and update timestamp
                    cache.find<IdTag>(id);
                }

                if (id % 5 == 0) {
                    const std::lock_guard lock{mutex};
                    cache.erase<IdTag>(id - 1);
                }
            }
        }));
    }

    for (auto& task : tasks) {
        task.Get();
    }

    const std::lock_guard lock{mutex};
    EXPECT_LE(cache.size(), 100);
}

}  // namespace

USERVER_NAMESPACE_END
