#include "tests/lru_basic_tests.h"

// #define LRU_CONTAINER_DEBUG__
#include <userver/multi_index_lru/lru_boost_list_container.h>

using namespace USERVER_NAMESPACE;

int main() {
    test_lru_users<lru_boost_list::LRUCacheContainer>();
    test_lru_products<lru_boost_list::LRUCacheContainer>();
    std::cout << "all tests success" << std::endl; 
    return 0;
}