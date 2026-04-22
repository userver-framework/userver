#include <userver/multi-index-lru/container.hpp>
#include <userver/utest/utest.hpp>

#include <string>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

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

UTEST_F(LRUUsersTest, BasicOperations) {
    UserCache cache(3);  // capacity == 3

    // Test insertion
    cache.emplace(User{.id = 1, .email = "alice@test.com", .name = "Alice"});
    cache.emplace(User{.id = 2, .email = "bob@test.com", .name = "Bob"});
    cache.emplace(User{.id = 3, .email = "charlie@test.com", .name = "Charlie"});

    EXPECT_EQ(cache.size(), 3);

    // Test find by id
    auto by_id = cache.find<IdTag, int>(1);
    EXPECT_NE(by_id, cache.end<IdTag>());
    EXPECT_EQ(by_id->name, "Alice");

    // Test find by email
    auto by_email = cache.find<EmailTag, std::string>("bob@test.com");
    EXPECT_NE(by_email, cache.end<EmailTag>());
    EXPECT_EQ(by_email->id, 2);

    // Test find by name
    auto by_name = cache.find<NameTag, std::string>("Charlie");
    EXPECT_NE(by_name, cache.end<NameTag>());
    EXPECT_EQ(by_name->email, "charlie@test.com");

    auto it = cache.find<EmailTag, std::string>("alice@test.com");
    EXPECT_NE(it, cache.end<EmailTag>());
}

UTEST_F(LRUUsersTest, LRUEviction) {
    UserCache cache(3);

    cache.emplace(User{.id = 1, .email = "alice@test.com", .name = "Alice"});
    cache.emplace(User{.id = 2, .email = "bob@test.com", .name = "Bob"});
    cache.emplace(User{.id = 3, .email = "charlie@test.com", .name = "Charlie"});

    // Access Alice and Charlie to make them recently used
    cache.find<IdTag>(1);
    cache.find<IdTag>(3);

    // Add fourth element - Bob should be evicted
    cache.emplace(User{.id = 4, .email = "david@test.com", .name = "David"});

    EXPECT_FALSE((cache.contains<IdTag>(2)));  // Bob evicted
    EXPECT_TRUE((cache.contains<IdTag>(1)));   // Alice remains
    EXPECT_TRUE((cache.contains<IdTag>(3)));   // Charlie remains
    EXPECT_TRUE((cache.contains<IdTag>(4)));   // David added
}

UTEST_F(LRUUsersTest, GetNoUpdateDoesNotChangeLru) {
    UserCache cache(3);

    cache.emplace(User{.id = 1, .email = "alice@test.com", .name = "Alice"});
    cache.emplace(User{.id = 2, .email = "bob@test.com", .name = "Bob"});
    cache.emplace(User{.id = 3, .email = "charlie@test.com", .name = "Charlie"});

    auto it = cache.find_no_update<IdTag>(1);  // without updating
    EXPECT_NE(it, cache.end<IdTag>());
    EXPECT_EQ(it->name, "Alice");

    cache.emplace(User{.id = 4, .email = "david@test.com", .name = "David"});

    EXPECT_FALSE((cache.contains<IdTag>(1)));  // evicted
    EXPECT_TRUE((cache.contains<IdTag>(2)));   // remains
    EXPECT_TRUE((cache.contains<IdTag>(3)));   // remains
    EXPECT_TRUE((cache.contains<IdTag>(4)));   // added
}

UTEST_F(LRUUsersTest, EqualRangeUpdatesLruForAllMatches) {
    UserCache cache(4);

    cache.emplace(User{.id = 1, .email = "john1@test.com", .name = "John"});
    cache.emplace(User{.id = 2, .email = "john2@test.com", .name = "John"});
    cache.emplace(User{.id = 3, .email = "alice@test.com", .name = "Alice"});
    cache.emplace(User{.id = 4, .email = "bob@test.com", .name = "Bob"});

    auto [begin, end] = cache.equal_range<NameTag, std::string>("John");
    int count = 0;
    for (auto it = begin; it != end; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);

    cache.emplace(User{.id = 5, .email = "eve@test.com", .name = "Eve"});

    EXPECT_TRUE((cache.contains<IdTag>(1)));   // remains
    EXPECT_TRUE((cache.contains<IdTag>(2)));   // remains
    EXPECT_FALSE((cache.contains<IdTag>(3)));  // evicted
    EXPECT_TRUE((cache.contains<IdTag>(4)));   // remains
    EXPECT_TRUE((cache.contains<IdTag>(5)));   // added
}

UTEST_F(LRUUsersTest, EqualRangeNoUpdateDoesNotChangeLru) {
    UserCache cache(4);

    cache.emplace(User{.id = 1, .email = "john1@test.com", .name = "John"});
    cache.emplace(User{.id = 2, .email = "john2@test.com", .name = "John"});
    cache.emplace(User{.id = 3, .email = "alice@test.com", .name = "Alice"});
    cache.emplace(User{.id = 4, .email = "bob@test.com", .name = "Bob"});

    auto [begin, end] = cache.equal_range_no_update<NameTag, std::string>("John");
    int count = 0;
    for (auto it = begin; it != end; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);

    cache.emplace(User{.id = 5, .email = "eve@test.com", .name = "Eve"});

    EXPECT_FALSE((cache.contains<IdTag>(1)));  // evicted
    EXPECT_TRUE((cache.contains<IdTag>(2)));   // remains
    EXPECT_TRUE((cache.contains<IdTag>(3)));   // remains
    EXPECT_TRUE((cache.contains<IdTag>(4)));   // remains
    EXPECT_TRUE((cache.contains<IdTag>(5)));   // added
}

UTEST_F(LRUUsersTest, EqualRangeWorksWithEmptyRange) {
    UserCache cache(3);
    cache.emplace(User{.id = 1, .email = "alice@test.com", .name = "Alice"});

    auto [begin, end] = cache.equal_range<NameTag, std::string>("Nonexistent");
    EXPECT_EQ(begin, end);
}

UTEST_F(LRUUsersTest, EqualRangeNoUpdateWorksWithEmptyRange) {
    UserCache cache(3);
    cache.emplace(User{.id = 1, .email = "alice@test.com", .name = "Alice"});

    auto [begin, end] = cache.equal_range_no_update<NameTag, std::string>("Nonexistent");
    EXPECT_EQ(begin, end);
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

UTEST_F(ProductsTest, BasicProductOperations) {
    ProductCache cache(2);

    cache.emplace(Product{.sku = "A1", .name = "Laptop", .price = 999.99});
    cache.emplace(Product{.sku = "A2", .name = "Mouse", .price = 29.99});

    auto laptop = cache.find<SkuTag, std::string>("A1");
    EXPECT_NE(laptop, cache.end<SkuTag>());
    EXPECT_EQ(laptop->name, "Laptop");
}

UTEST_F(ProductsTest, ProductEviction) {
    ProductCache cache(2);

    cache.emplace(Product{.sku = "A1", .name = "Laptop", .price = 999.99});
    cache.emplace(Product{.sku = "A2", .name = "Mouse", .price = 29.99});

    // A1 was used, so A2 should be ousted when adding A3
    cache.find<SkuTag>("A1");
    cache.emplace(Product{.sku = "A3", .name = "Keyboard", .price = 79.99});

    EXPECT_TRUE((cache.contains<SkuTag, std::string>("A1")));   // used
    EXPECT_TRUE((cache.contains<SkuTag, std::string>("A3")));   // new
    EXPECT_FALSE((cache.contains<SkuTag, std::string>("A2")));  // ousted

    EXPECT_NE(cache.find<NameTag>("Keyboard"), cache.end<NameTag>());
    EXPECT_EQ(cache.find<NameTag>("Mouse"), cache.end<NameTag>());
}

class ProductsTestWithAllocator : public ProductsTest {
protected:
    class Counter {
    public:
        static std::atomic<size_t> count;
        static void increment() { count++; }
        static size_t get() { return count.load(); }
        static void reset() { count = 0; }
    };

    template <typename T>
    class CountingAllocator : public std::allocator<T> {
    public:
        CountingAllocator() = default;
        template <typename U>
        CountingAllocator(const CountingAllocator<U>&) {}

        T* allocate(size_t n) {
            Counter::increment();
            return std::allocator<T>::allocate(n);
        }

        static size_t get_count() { return Counter::get(); }
        static void reset_count() { Counter::reset(); }

        template <typename U>
        struct rebind {  // NOLINT(readability-identifier-naming)
            using other = CountingAllocator<U>;
        };
    };

    using ProductCache = multi_index_lru::Container<
        Product,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<SkuTag>,
                boost::multi_index::member<Product, std::string, &Product::sku>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<NameTag>,
                boost::multi_index::member<Product, std::string, &Product::name>>>,
        CountingAllocator<Product>>;
};

std::atomic<size_t> ProductsTestWithAllocator::Counter::count{0};

UTEST_F(ProductsTestWithAllocator, AllocationsCheck) {
    ProductCache cache(20);

    for (int i = 0; i < 1000; ++i) {
        cache.insert(Product{.sku = "A" + std::to_string(i), .name = "Laptop_" + std::to_string(i), .price = 999.99});
    }
    const auto first_allocations_count = ProductsTestWithAllocator::CountingAllocator<int>::get_count();

    cache.clear();
    for (int i = 0; i < 1000; ++i) {
        cache.insert(Product{.sku = "A" + std::to_string(i), .name = "Laptop_" + std::to_string(i), .price = 999.99});
    }

    // no extra allocations since nodes are being reused
    EXPECT_EQ(first_allocations_count, ProductsTestWithAllocator::CountingAllocator<int>::get_count());

    cache.insert(Product{.sku = "B_0", .name = "C_0", .price = 999.99});
    cache.erase(cache.find<NameTag>("C_0"));
    cache.insert(Product{.sku = "B_1", .name = "C_1", .price = 999.99});
    cache.erase(cache.find<SkuTag>("B_1"));
    cache.insert(Product{.sku = "B_2", .name = "C_2", .price = 999.99});

    // no extra allocations since nodes are being reused
    EXPECT_EQ(first_allocations_count, ProductsTestWithAllocator::CountingAllocator<int>::get_count());
}

UTEST(Snippet, SimpleUsage) {
    struct MyValueT {
        std::string key;
        int val;
    };

    struct MyTag {};

    MyValueT my_value{.key = "some_key", .val = 1};
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
