#include "src/tests/lru_basic_tests.h"
#include "src/benchmarks/lru_basic_benchmarks.h"
#include "src/benchmarks/lru_google_benchmarks.h"

// #define LRU_CONTAINER_DEBUG__
#include "src/implements/examples/lru_time_index_container.h"
#include "src/implements/examples/lru_list_container.h"
#include "src/implements/actual/lru_boost_list_container.h"


int main() {
    test_lru_users<lru_time_index::LRUCacheContainer_TimeIndex>();
    test_lru_products<lru_time_index::LRUCacheContainer_TimeIndex>();
    std::cout << "all tests success" << std::endl;

    test_lru_users<lru_list::LRUCacheContainer_List>();
    test_lru_products<lru_list::LRUCacheContainer_List>();
    std::cout << "all tests success" << std::endl;

    test_lru_users<lru_boost_list::LRUCacheContainer_BoostList>();
    test_lru_products<lru_boost_list::LRUCacheContainer_BoostList>();
    std::cout << "all tests success" << std::endl; 

    benchmark::simple_benchmark<lru_list::LRUCacheContainer_List>("list_output.txt");
    benchmark::simple_benchmark<lru_time_index::LRUCacheContainer_TimeIndex>("time_index_output.txt");
    benchmark::simple_benchmark<lru_boost_list::LRUCacheContainer_BoostList>("boost_list_output.txt");

    benchmark::google_benchmark<lru_list::LRUCacheContainer_List>();
    benchmark::google_benchmark<lru_time_index::LRUCacheContainer_TimeIndex>();
    benchmark::google_benchmark<lru_boost_list::LRUCacheContainer_BoostList>();

    benchmark::google_benchmark_init("google_output.txt");
    benchmark::google_benchmark_run();
    return 0;
}