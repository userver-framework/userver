#include <userver/multi-index-lru/container.hpp>

#include <string>

#include <gtest/gtest.h>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

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
