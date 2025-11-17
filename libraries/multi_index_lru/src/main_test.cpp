#include <iostream>
#include <string>

#include <gtest/gtest.h>
#include <boost/multi_index/member.hpp>

#include <userver/multi_index_lru/container.hpp>

using namespace USERVER_NAMESPACE;

namespace {

class LRUUsersTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    struct id_tag {};
    struct email_tag {};
    struct name_tag {};

    struct User {
        int id;
        std::string email;
        std::string name;
        
        bool operator==(const User& other) const {
            return id == other.id && email == other.email && name == other.name;
        }
    };

    using UserCache = multi_index_lru::LRUCacheContainer<
        User,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<id_tag>,
                boost::multi_index::member<User, int, &User::id>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<email_tag>, 
                boost::multi_index::member<User, std::string, &User::email>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<name_tag>, 
                boost::multi_index::member<User, std::string, &User::name>>
        >
    >;
};

TEST_F(LRUUsersTest, BasicOperations) {
    UserCache cache(3); // capacity == 3
    
    // Test insertion
    cache.emplace(User{1, "alice@test.com", "Alice"});
    cache.emplace(User{2, "bob@test.com", "Bob"});
    cache.emplace(User{3, "charlie@test.com", "Charlie"});
    
    EXPECT_EQ(cache.size(), 3);
    
    // Test find by id
    auto by_id = cache.template get<id_tag>().find(1);
    ASSERT_NE(by_id, cache.template get<id_tag>().end());
    EXPECT_EQ(by_id->get().name, "Alice");
    
    // Test find by email
    auto by_email = cache.template get<email_tag>().find("bob@test.com");
    ASSERT_NE(by_email, cache.template get<email_tag>().end());
    EXPECT_EQ(by_email->get().id, 2);
    
    // Test find by name
    auto by_name = cache.template get<name_tag>().find("Charlie");
    ASSERT_NE(by_name, cache.template get<name_tag>().end());
    EXPECT_EQ(by_name->get().email, "charlie@test.com");
    
    // Test template find method
    auto it = cache.template find<email_tag, std::string>("alice@test.com");
    EXPECT_NE(it, cache.template get<email_tag>().end());
}

TEST_F(LRUUsersTest, LRUEviction) {
    UserCache cache(3);
    
    cache.emplace(User{1, "alice@test.com", "Alice"});
    cache.emplace(User{2, "bob@test.com", "Bob"});
    cache.emplace(User{3, "charlie@test.com", "Charlie"});
    
    // Access Alice and Charlie to make them recently used
    cache.template get<id_tag>().find(1);
    cache.template get<id_tag>().find(3);

    // Add fourth element - Bob should be evicted
    cache.emplace(User{4, "david@test.com", "David"});
    
    EXPECT_FALSE(cache.template contains<id_tag, int>(2)); // Bob evicted
    EXPECT_TRUE(cache.template contains<id_tag, int>(1));  // Alice remains
    EXPECT_TRUE(cache.template contains<id_tag, int>(3));  // Charlie remains  
    EXPECT_TRUE(cache.template contains<id_tag, int>(4));  // David added
}

class ProductsTest : public ::testing::Test {
protected:
    struct sku_tag {};
    struct name_tag {};

    struct Product {
        std::string sku;
        std::string name;
        double price;
        
        bool operator==(const Product& other) const {
            return sku == other.sku && name == other.name && price == other.price;
        }
    };

    using ProductCache = multi_index_lru::LRUCacheContainer<
        Product,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<sku_tag>,
                boost::multi_index::member<Product, std::string, &Product::sku>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<name_tag>,
                boost::multi_index::member<Product, std::string, &Product::name>>
        >
    >;
};

TEST_F(ProductsTest, BasicProductOperations) {
    ProductCache cache(2);
    
    cache.emplace(Product{"A1", "Laptop", 999.99});
    cache.emplace(Product{"A2", "Mouse", 29.99});
    
    auto laptop = cache.template find<sku_tag, std::string>("A1");
    ASSERT_NE(laptop, cache.template get<sku_tag>().end());
    EXPECT_EQ(laptop->get().name, "Laptop");
}

TEST_F(ProductsTest, ProductEviction) {
    ProductCache cache(2);
    
    cache.emplace(Product{"A1", "Laptop", 999.99});
    cache.emplace(Product{"A2", "Mouse", 29.99});
    
    // A1 was used, so A2 should be ousted when adding A3
    cache.template get<sku_tag>().find("A1");
    cache.emplace(Product{"A3", "Keyboard", 79.99});
    
    EXPECT_TRUE(cache.template contains<sku_tag, std::string>("A1"));  // used
    EXPECT_TRUE(cache.template contains<sku_tag, std::string>("A3"));  // new
    EXPECT_FALSE(cache.template contains<sku_tag, std::string>("A2")); // ousted
    
    EXPECT_NE(cache.template get<name_tag>().find("Keyboard"), cache.template get<name_tag>().end());
    EXPECT_EQ(cache.template get<name_tag>().find("Mouse"), cache.template get<name_tag>().end());
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int test_result = RUN_ALL_TESTS();
    
    if (test_result == 0) {
        std::cout << "All tests passed!" << std::endl;
    }
    
    return test_result;
}