#include <userver/multi-index-lru/expirable_container.hpp>
#include <userver/utils/rand.hpp>
#include <userver/engine/run_standalone.hpp>

#include <random>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>

#include <benchmark/benchmark.h>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

USERVER_NAMESPACE_BEGIN

namespace benchmarks {

const std::size_t kOperationsNumber = 100000;
const int kMaxIdSize = 50000;

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

namespace {

User GenerateUser() {
    return User{
        utils::RandRange<int>(0, kMaxIdSize),
        "email" + std::to_string(utils::RandRange<int>(0, kMaxIdSize)),
        "name" + std::to_string(utils::RandRange<int>(0, kMaxIdSize))
    };
}

int GenerateId() {
    return utils::RandRange<int>(0, kMaxIdSize);
}

std::string GenerateName() {
    return "name" + std::to_string(utils::RandRange<int>(0, kMaxIdSize));
}

std::string GenerateEmail() {
    return "email" + std::to_string(utils::RandRange<int>(0, kMaxIdSize));
}

}  // namespace

using UserCache = multi_index_lru::ExpirableContainer<
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

static void ExpirableFindEmplaceMix(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
    const std::size_t cache_size = state.range(0);

    UserCache cache(cache_size, std::chrono::seconds(5));

    for (std::size_t i = 0; i < cache_size; ++i) {
        cache.insert(GenerateUser());
    }

    const std::size_t read_ops = kOperationsNumber * 4 / 5;
    const std::size_t write_ops = kOperationsNumber / 5;

    std::vector<std::string> names(read_ops);
    std::vector<std::string> emails(read_ops);
    std::vector<int> ids(read_ops);
    std::vector<User> users(write_ops);

    for (std::size_t i = 0; i < read_ops; ++i) {
        names[i] = GenerateName();
        emails[i] = GenerateEmail();
        ids[i] = GenerateId();
    }

    for (std::size_t i = 0; i < write_ops; ++i) {
        users[i] = GenerateUser();
    }

    for (auto _ : state) {
        for (std::size_t i = 0; i < read_ops; ++i) {
            cache.find<NameTag, std::string>(names[i]);
            cache.find<EmailTag, std::string>(emails[i]);
            cache.find<IdTag, int>(ids[i]);
        }

        for (std::size_t i = 0; i < write_ops; ++i) { 
            cache.insert(users[i]);
        }
    }
    });
}

BENCHMARK(ExpirableFindEmplaceMix)
    ->RangeMultiplier(10)
    ->Range(10, 1'000'000);


static void PrepareCache(UserCache& cache, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        cache.insert(GenerateUser());
    }
}

static void ExpirableGetOperations(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
    const std::size_t cache_size = state.range(0);

    UserCache cache(cache_size, std::chrono::minutes(10));
    PrepareCache(cache, cache_size);

    
    for (auto _ : state) {
        state.PauseTiming();

        std::vector<std::string> names(kOperationsNumber);
        std::vector<std::string> emails(kOperationsNumber);
        std::vector<int> ids(kOperationsNumber);

        for (std::size_t i = 0; i < kOperationsNumber; ++i) {
            names[i] = GenerateName();
            emails[i] = GenerateEmail();
            ids[i] = GenerateId();
        }

        state.ResumeTiming();

        for (std::size_t i = 0; i < kOperationsNumber; ++i) {
            benchmark::DoNotOptimize(cache.find<NameTag, std::string>(names[i]));
            benchmark::DoNotOptimize(cache.find<EmailTag, std::string>(emails[i]));
            benchmark::DoNotOptimize(cache.find<IdTag, int>(ids[i]));
        }
    }

    state.SetItemsProcessed(state.iterations() * kOperationsNumber * 3);
    state.SetComplexityN(cache_size);
    });
}

BENCHMARK(ExpirableGetOperations)
    ->RangeMultiplier(10)
    ->Range(100, 1'000'000);


static void ExpirableEmplaceOperations(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
    const std::size_t cache_size = state.range(0);

    UserCache cache(cache_size, std::chrono::minutes(10));
    PrepareCache(cache, cache_size);

    for (auto _ : state) {
        state.PauseTiming();

        std::vector<User> users(kOperationsNumber);
        for (std::size_t i = 0; i < kOperationsNumber; ++i) {
            users[i] = GenerateUser();
        }

        state.ResumeTiming();

        for (std::size_t i = 0; i < kOperationsNumber; ++i) {
            cache.insert(users[i]);
        }
    }

    state.SetItemsProcessed(state.iterations() * kOperationsNumber);
    state.SetComplexityN(cache_size);
    });
}

BENCHMARK(ExpirableEmplaceOperations)
    ->RangeMultiplier(10)
    ->Range(100, 1'000'000);

}  // namespace benchmarks

USERVER_NAMESPACE_END