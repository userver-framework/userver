#include "tests/lru_basic_tests.h"
#include "benchmarks/lru_basic_benchmarks.h"
#include "benchmarks/lru_google_benchmarks.h"

// #define LRU_CONTAINER_DEBUG__
#include <userver/multi_index_lru/implements/examples/lru_time_index_container.h>
#include <userver/multi_index_lru/implements/examples/lru_list_container.h>
#include <userver/multi_index_lru/lru_boost_list_container.h>

using namespace USERVER_NAMESPACE;

int main() {
    test_lru_users<lru_time_index::LRUCacheContainer_TimeIndex>();
    test_lru_products<lru_time_index::LRUCacheContainer_TimeIndex>();
    std::cout << "all tests success" << std::endl;

    test_lru_users<lru_list::LRUCacheContainer_List>();
    test_lru_products<lru_list::LRUCacheContainer_List>();
    std::cout << "all tests success" << std::endl;

    test_lru_users<lru_boost_list::LRUCacheContainer>();
    test_lru_products<lru_boost_list::LRUCacheContainer>();
    std::cout << "all tests success" << std::endl; 

    benchmarks::simple_benchmark<lru_list::LRUCacheContainer_List>("list_output.txt");
    benchmarks::simple_benchmark<lru_time_index::LRUCacheContainer_TimeIndex>("time_index_output.txt");
    benchmarks::simple_benchmark<lru_boost_list::LRUCacheContainer>("boost_list_output.txt");

    benchmarks::google_benchmark<lru_list::LRUCacheContainer_List>();
    benchmarks::google_benchmark<lru_time_index::LRUCacheContainer_TimeIndex>();
    benchmarks::google_benchmark<lru_boost_list::LRUCacheContainer>();

    benchmarks::google_benchmark_init("google_output.txt");
    benchmarks::google_benchmark_run();
    return 0;
}