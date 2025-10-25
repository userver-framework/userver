#pragma once

#include <benchmark/benchmark.h>
#include "benchmarks_resourses.h"

USERVER_NAMESPACE_BEGIN

namespace benchmarks {

template<
    template<typename, typename, typename> class LRUCacheContainer
>
class LRUCacheBenchmark {
private:
    using UserCache = LRUCacheContainer<
        User,
        indexed_by<
            ordered_unique<tag<id_tag>, member<User, int, &User::id>>,
            ordered_unique<tag<email_tag>, member<User, std::string, &User::email>>,
            ordered_non_unique<tag<name_tag>, member<User, std::string, &User::name>>
        >,
        std::allocator<User>
    >;

    static void prepare_cache(UserCache& cache, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            cache.emplace(generator::generate_user());
        }
    }

public:
    static void BM_GetOperations(::benchmark::State& state) {
        const size_t cache_size = state.range(0);
        const size_t operations_count = state.range(1);
        
        UserCache cache(cache_size);
        prepare_cache(cache, cache_size);

        for (auto _ : state) {
            state.PauseTiming();
            std::vector<std::string> names, emails;
            std::vector<int> ids;
            for (size_t i = 0; i < operations_count; ++i) {
                names.push_back(generator::generate_name());
                emails.push_back(generator::generate_email());
                ids.push_back(generator::generate_id());
            }
            state.ResumeTiming();

            for (size_t i = 0; i < operations_count; ++i) {
                ::benchmark::DoNotOptimize(cache.template find<name_tag, std::string>(names[i]));
                ::benchmark::DoNotOptimize(cache.template find<email_tag, std::string>(emails[i]));
                ::benchmark::DoNotOptimize(cache.template find<id_tag, int>(ids[i]));
            }
        }

        state.SetItemsProcessed(state.iterations() * operations_count * 3);
        state.SetComplexityN(cache_size);
    }

    static void BM_EmplaceOperations(::benchmark::State& state) {
        const size_t cache_size = state.range(0);
        const size_t operations_count = state.range(1);
        
        UserCache cache(cache_size);
        prepare_cache(cache, cache_size);

        for (auto _ : state) {
            state.PauseTiming();
            std::vector<User> users;
            for (size_t i = 0; i < operations_count; ++i) {
                users.push_back(generator::generate_user());
            }
            state.ResumeTiming();

            for (size_t i = 0; i < operations_count; ++i) {
                cache.emplace(users[i]);
            }
        }

        state.SetItemsProcessed(state.iterations() * operations_count);
        state.SetComplexityN(cache_size);
    }
};

void google_benchmark_init(std::string&& output_filename) {
    std::vector<char*> args;
    std::string prog_name = "benchmark";
    args.push_back(prog_name.data());
    std::string out_arg = "--benchmark_out=" + output_filename;
    args.push_back(out_arg.data());
    std::string format_arg = "--benchmark_out_format=json";
    args.push_back(format_arg.data());
    int argc = args.size();
    ::benchmark::Initialize(&argc, args.data());
}

void google_benchmark_run() {
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::ClearRegisteredBenchmarks();
    ::benchmark::Shutdown();
}

template<
    template<typename, typename, typename> class LRUCacheContainer
>
void google_benchmark() {
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

    for (auto size : CACHE_SIZES) {
        ::benchmark::RegisterBenchmark(
            "GetOperations", 
            &LRUCacheBenchmark<LRUCacheContainer>::BM_GetOperations
        )->Args({size, OPERATIONS_NUMBER})->Unit(::benchmark::kMicrosecond);
    }

    for (auto size : CACHE_SIZES) {
        ::benchmark::RegisterBenchmark(
            "EmplaceOperations", 
            &LRUCacheBenchmark<LRUCacheContainer>::BM_EmplaceOperations
        )->Args({size, OPERATIONS_NUMBER})->Unit(::benchmark::kMicrosecond);
    }
}
} // benchmarks
USERVER_NAMESPACE_END