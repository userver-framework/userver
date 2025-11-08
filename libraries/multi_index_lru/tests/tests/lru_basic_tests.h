#pragma once

#include <iostream>
#include <string>
#include <cassert>

#include "../lru_container_concept.h"

USERVER_NAMESPACE_BEGIN

namespace {

using namespace boost::multi_index;

template<
    template<typename, typename, typename> class LRUCacheContainer
>
void test_lru_users() {

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

    using UserCache = LRUCacheContainer<
        User,
        indexed_by<
            ordered_unique<tag<id_tag>, member<User, int, &User::id>>,
            ordered_unique<tag<email_tag>, member<User, std::string, &User::email>>,
            ordered_non_unique<tag<name_tag>, member<User, std::string, &User::name>>
        >,
        std::allocator<User>
    >;

    lru_concept_assert_for_one_tag(UserCache, id_tag, int, User);
    lru_concept_assert_for_one_tag(UserCache, email_tag, std::string, User);
    lru_concept_assert_for_one_tag(UserCache, name_tag, std::string, User);
    
    UserCache cache(3); // capacity == 3
    
    cache.emplace(User{1, "alice@test.com", "Alice"});
    cache.emplace(User{2, "bob@test.com", "Bob"});
    cache.emplace(User{3, "charlie@test.com", "Charlie"});
    
    // find by id
    [[maybe_unused]] auto by_id = cache.template get<id_tag>().find(1);
    assert((by_id != cache.template get<id_tag>().end()));
    assert((by_id->get().name == "Alice"));
    
    // find by email
    [[maybe_unused]] auto by_email = cache.template get<email_tag>().find("bob@test.com");
    assert((by_email != cache.template get<email_tag>().end()));
    assert((by_email->get().id == 2));
    
    //find by name
    [[maybe_unused]] auto by_name = cache.template get<name_tag>().find("Charlie");
    assert((by_name != cache.template get<name_tag>().end()));
    assert((by_name->get().email == "charlie@test.com"));
    
    //find by email
    [[maybe_unused]] auto it = cache.template find<email_tag, std::string>("alice@test.com");
    assert((it != cache.template get<email_tag>().end()));
    
    //find by id
    cache.template find<id_tag, int>(1); 

    // capacity == 3, Alice, Charlie was recently used -> Bob will be ousted 
    cache.emplace(User{4, "david@test.com", "David"}); 
    
    assert((!cache.template contains<id_tag, int>(2))); // Bob outsed
    assert((cache.template contains<id_tag, int>(1))); 
    assert((cache.template contains<id_tag, int>(3))); 
    assert((cache.template contains<id_tag, int>(4))); 

    std::cout << "test_lru_users correct" << std::endl;
    
}

template <template <typename, typename, typename> typename LRUCacheContainer>
void test_lru_products() {

    struct Product {
        std::string sku;
        std::string name;
        double price;
    };
    
    struct sku_tag {};
    struct name_tag {};
    
    using ProductCache = LRUCacheContainer<
        Product,
        indexed_by<
            ordered_unique<tag<sku_tag>, member<Product, std::string, &Product::sku>>,
            ordered_unique<tag<name_tag>, member<Product, std::string, &Product::name>>
        >,
        std::allocator<Product>
    >;

    lru_concept_assert_for_one_tag(ProductCache, sku_tag, std::string, Product);
    
    ProductCache cache(2);
    
    cache.emplace(Product{"A1", "Laptop", 999.99});
    cache.emplace(Product{"A2", "Mouse", 29.99});
    
    [[maybe_unused]] auto laptop = cache.template find<sku_tag, std::string>("A1");
    assert((laptop != cache.template get<sku_tag>().end()));
    
    // A1 was used, so A2 should be ousted
    cache.emplace(Product{"A3", "Keyboard", 79.99});
    
    assert((cache.template contains<sku_tag, std::string>("A1"))); // used
    assert((cache.template contains<sku_tag, std::string>("A3"))); // new
    assert((!cache.template contains<sku_tag, std::string>("A2"))); // ousted
    
    assert((cache.template get<name_tag>().find("Keyboard") != cache.template get<name_tag>().end()));
    assert((cache.template get<name_tag>().find("Mouse") == cache.template get<name_tag>().end()));
    
    std::cout << "test_lru_products correct" << std::endl;
}
}

USERVER_NAMESPACE_END
