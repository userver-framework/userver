#include "benchmarks/lru_basic_benchmarks.h"
#include "benchmarks/lru_google_benchmarks.h"

// #define LRU_CONTAINER_DEBUG__
#include <userver/multi_index_lru/lru_boost_list_container.h>

using namespace USERVER_NAMESPACE;

int main() {
    benchmarks::simple_benchmark<multi_index_lru::LRUCacheContainer>("boost_list_output.txt");
    benchmarks::google_benchmark<multi_index_lru::LRUCacheContainer>();

    benchmarks::google_benchmark_init("google_output.txt");
    benchmarks::google_benchmark_run();
    return 0;
}