#include "tests/lru_basic_tests.h"
#include "benchmarks/lru_basic_benchmarks.h"
#include "benchmarks/lru_google_benchmarks.h"

// #define LRU_CONTAINER_DEBUG__
#include <userver/multi_index_lru/lru_boost_list_container.h>

using namespace USERVER_NAMESPACE;

int main() {
    test_lru_users<lru_boost_list::LRUCacheContainer>();
    test_lru_products<lru_boost_list::LRUCacheContainer>();
    std::cout << "all tests success" << std::endl; 

    benchmarks::simple_benchmark<lru_boost_list::LRUCacheContainer>("boost_list_output.txt");
    benchmarks::google_benchmark<lru_boost_list::LRUCacheContainer>();

    benchmarks::google_benchmark_init("google_output.txt");
    benchmarks::google_benchmark_run();
    return 0;
}