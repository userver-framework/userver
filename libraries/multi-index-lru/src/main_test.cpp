#include <userver/multi-index-lru/container.hpp>
#include <userver/multi-index-lru/expirable_container.hpp>

#include <string>

#include <gtest/gtest.h>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class LRUUsersTest : public ::testing::Test {
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

    using UserCache = multi_index_lru::Container<
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

TEST_F(LRUUsersTest, BasicOperations) {
    UserCache cache(3);  // capacity == 3

    // Test insertion
    cache.emplace(User{1, "alice@test.com", "Alice"});
    cache.emplace(User{2, "bob@test.com", "Bob"});
    cache.emplace(User{3, "charlie@test.com", "Charlie"});

    EXPECT_EQ(cache.size(), 3);

    // Test find by id
    auto by_id = cache.find<IdTag, int>(1);
    ASSERT_NE(by_id, cache.end<IdTag>());
    EXPECT_EQ(by_id->name, "Alice");

    // Test find by email
    auto by_email = cache.find<EmailTag, std::string>("bob@test.com");
    ASSERT_NE(by_email, cache.end<EmailTag>());
    EXPECT_EQ(by_email->id, 2);

    // Test find by name
    auto by_name = cache.find<NameTag, std::string>("Charlie");
    ASSERT_NE(by_name, cache.end<NameTag>());
    EXPECT_EQ(by_name->email, "charlie@test.com");

    // Test template find method
    auto it = cache.find<EmailTag, std::string>("alice@test.com");
    EXPECT_NE(it, cache.end<EmailTag>());
}

TEST_F(LRUUsersTest, LRUEviction) {
    UserCache cache(3);

    cache.emplace(User{1, "alice@test.com", "Alice"});
    cache.emplace(User{2, "bob@test.com", "Bob"});
    cache.emplace(User{3, "charlie@test.com", "Charlie"});

    // Access Alice and Charlie to make them recently used
    cache.find<IdTag>(1);
    cache.find<IdTag>(3);

    // Add fourth element - Bob should be evicted
    cache.emplace(User{4, "david@test.com", "David"});

    EXPECT_FALSE((cache.contains<IdTag>(2)));  // Bob evicted
    EXPECT_TRUE((cache.contains<IdTag>(1)));   // Alice remains
    EXPECT_TRUE((cache.contains<IdTag>(3)));   // Charlie remains
    EXPECT_TRUE((cache.contains<IdTag>(4)));   // David added
}

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
}

TEST_F(ExpirableUsersTest, LRUEviction) {
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
}

TEST_F(ExpirableUsersTest, TTLExpiration) {
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
}

TEST_F(ExpirableUsersTest, TTLRefreshOnAccess) {
    using namespace std::chrono_literals;
    
    UserCacheExpirable cache(100, 200ms);
    
    cache.insert(User{1, "alice@test.com", "Alice"});
    
    // Wait a bit but not enough to expire
    std::this_thread::sleep_for(80ms);
    
    // Access should refresh TTL
    EXPECT_TRUE(cache.contains<IdTag>(1));
    
    // Wait again - should still be alive due to refresh
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(cache.contains<IdTag>(1));
    
    // Wait for full TTL from last access
    std::this_thread::sleep_for(200ms);
    EXPECT_FALSE(cache.contains<IdTag>(1));
}

TEST_F(ExpirableUsersTest, EraseOperations) {
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

TEST_F(ExpirableUsersTest, SetCapacity) {
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

TEST_F(ExpirableUsersTest, Clear) {
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

TEST_F(ExpirableUsersTest, ThreadSafetyBasic) {
    UserCacheExpirable cache(100, std::chrono::seconds(10));
    
    constexpr int kThreads = 4;
    constexpr int kIterations = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&cache, t]() {
            for (int i = 0; i < kIterations; ++i) {
                int id = t * kIterations + i;
                cache.insert(User{id, std::to_string(id) + "@test.com", "User" + std::to_string(id)});
                
                // Concurrent reads
                if (id % 3 == 0) {
                    cache.find<IdTag>(id);
                    cache.contains<IdTag>(id);
                }
                
                // Concurrent erase
                if (id % 5 == 0) {
                    cache.erase<IdTag>(id - 1);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should not crash and size should be reasonable
    EXPECT_LE(cache.size(), 100);  // Due to capacity limit
}

class ProductsTest : public ::testing::Test {
protected:
    struct SkuTag {};
    struct NameTag {};

    struct Product {
        std::string sku;
        std::string name;
        double price;

        bool operator==(const Product& other) const {
            return sku == other.sku && name == other.name && price == other.price;
        }
    };

    using ProductCache = multi_index_lru::Container<
        Product,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<SkuTag>,
                boost::multi_index::member<Product, std::string, &Product::sku>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<NameTag>,
                boost::multi_index::member<Product, std::string, &Product::name>>>>;
};

TEST_F(ProductsTest, BasicProductOperations) {
    ProductCache cache(2);

    cache.emplace(Product{"A1", "Laptop", 999.99});
    cache.emplace(Product{"A2", "Mouse", 29.99});

    auto laptop = cache.find<SkuTag, std::string>("A1");
    ASSERT_NE(laptop, cache.end<SkuTag>());
    EXPECT_EQ(laptop->name, "Laptop");
}

TEST_F(ProductsTest, ProductEviction) {
    ProductCache cache(2);

    cache.emplace(Product{"A1", "Laptop", 999.99});
    cache.emplace(Product{"A2", "Mouse", 29.99});

    // A1 was used, so A2 should be ousted when adding A3
    cache.find<SkuTag>("A1");
    cache.emplace(Product{"A3", "Keyboard", 79.99});

    EXPECT_TRUE((cache.contains<SkuTag, std::string>("A1")));   // used
    EXPECT_TRUE((cache.contains<SkuTag, std::string>("A3")));   // new
    EXPECT_FALSE((cache.contains<SkuTag, std::string>("A2")));  // ousted

    EXPECT_NE(cache.find<NameTag>("Keyboard"), cache.end<NameTag>());
    EXPECT_EQ(cache.find<NameTag>("Mouse"), cache.end<NameTag>());
}

TEST(Snippet, SimpleUsage) {
    struct MyValueT {
        std::string key;
        int val;
    };

    struct MyTag {};

    MyValueT my_value{"some_key", 1};
    /// [Usage]
    using MyLruCache = multi_index_lru::Container<
        MyValueT,
        boost::multi_index::indexed_by<boost::multi_index::hashed_unique<
            boost::multi_index::tag<MyTag>,
            boost::multi_index::member<MyValueT, std::string, &MyValueT::key>>>>;

    MyLruCache cache(1000);  // Capacity of 1000 items
    cache.insert(my_value);
    auto it = cache.find<MyTag>("some_key");
    EXPECT_NE(it, cache.end<MyTag>());
    /// [Usage]
}

}  // namespace

USERVER_NAMESPACE_END
