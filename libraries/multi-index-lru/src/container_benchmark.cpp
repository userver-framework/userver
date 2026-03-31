#include <userver/engine/run_standalone.hpp>
#include <userver/multi-index-lru/container.hpp>
#include <userver/multi-index-lru/expirable_container.hpp>
#include <userver/utils/rand.hpp>

#include <chrono>
#include <string>
#include <vector>

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

    bool operator==(const User& other) const { return id == other.id && email == other.email && name == other.name; }
};

using UserIndices = boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<
        boost::multi_index::tag<IdTag>,
        boost::multi_index::member<User, int, &User::id>>,
    boost::multi_index::ordered_unique<
        boost::multi_index::tag<EmailTag>,
        boost::multi_index::member<User, std::string, &User::email>>,
    boost::multi_index::ordered_non_unique<
        boost::multi_index::tag<NameTag>,
        boost::multi_index::member<User, std::string, &User::name>>>;

// container types
using UserLruCache = multi_index_lru::Container<User, UserIndices>;
using UserExpirableCache = multi_index_lru::ExpirableContainer<User, UserIndices>;

// data generators
namespace {

User GenerateUser() {
    return User{
        utils::RandRange<int>(0, kMaxIdSize),
        "email" + std::to_string(utils::RandRange<int>(0, kMaxIdSize)),
        "name" + std::to_string(utils::RandRange<int>(0, kMaxIdSize))
    };
}

int GenerateId() { return utils::RandRange<int>(0, kMaxIdSize); }

std::string GenerateName() { return "name" + std::to_string(utils::RandRange<int>(0, kMaxIdSize)); }

std::string GenerateEmail() { return "email" + std::to_string(utils::RandRange<int>(0, kMaxIdSize)); }

template <typename Container>
void PrepareCache(Container& cache, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        cache.insert(GenerateUser());
    }
}

void PrepareReadData(
    std::vector<std::string>& names,
    std::vector<std::string>& emails,
    std::vector<int>& ids,
    std::size_t count
) {
    names.clear();
    emails.clear();
    ids.clear();
    names.reserve(count);
    emails.reserve(count);
    ids.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        names.push_back(GenerateName());
        emails.push_back(GenerateEmail());
        ids.push_back(GenerateId());
    }
}

void PrepareWriteData(std::vector<User>& users, std::size_t count) {
    users.clear();
    users.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        users.push_back(GenerateUser());
    }
}

}  // namespace

// template benchmark functions
template <typename Container>
void RunFindInsertMix(benchmark::State& state, Container& cache) {
    const std::size_t read_ops = kOperationsNumber * 4 / 5;
    const std::size_t write_ops = kOperationsNumber / 5;

    std::vector<std::string> names, emails;
    std::vector<int> ids;
    std::vector<User> users;

    PrepareReadData(names, emails, ids, read_ops);
    PrepareWriteData(users, write_ops);

    for ([[maybe_unused]] auto _ : state) {
        for (std::size_t i = 0; i < read_ops; ++i) {
            benchmark::DoNotOptimize(cache.template find<NameTag>(names[i]));
            benchmark::DoNotOptimize(cache.template find<EmailTag>(emails[i]));
            benchmark::DoNotOptimize(cache.template find<IdTag>(ids[i]));
        }

        for (std::size_t i = 0; i < write_ops; ++i) {
            cache.insert(users[i]);
        }
    }
}

template <typename Container>
void RunGetOperations(benchmark::State& state, Container& cache) {
    std::vector<std::string> names, emails;
    std::vector<int> ids;

    for (auto _ : state) {
        state.PauseTiming();
        PrepareReadData(names, emails, ids, kOperationsNumber);
        state.ResumeTiming();

        for (std::size_t i = 0; i < kOperationsNumber; ++i) {
            benchmark::DoNotOptimize(cache.template find<NameTag>(names[i]));
            benchmark::DoNotOptimize(cache.template find<EmailTag>(emails[i]));
            benchmark::DoNotOptimize(cache.template find<IdTag>(ids[i]));
        }
    }

    state.SetItemsProcessed(state.iterations() * kOperationsNumber * 3);
    state.SetComplexityN(state.range(0));
}

template <typename Container>
void RunInsertOperations(benchmark::State& state, Container& cache) {
    std::vector<User> users;

    for (auto _ : state) {
        state.PauseTiming();
        PrepareWriteData(users, kOperationsNumber);
        state.ResumeTiming();

        for (std::size_t i = 0; i < kOperationsNumber; ++i) {
            cache.insert(users[i]);
        }
    }

    state.SetItemsProcessed(state.iterations() * kOperationsNumber);
    state.SetComplexityN(state.range(0));
}

// Container's benchmarks
static void LruFindInsertMix(benchmark::State& state) {
    const std::size_t size = state.range(0);
    UserLruCache cache(size);
    PrepareCache(cache, size);
    RunFindInsertMix(state, cache);
}
BENCHMARK(LruFindInsertMix)->RangeMultiplier(10)->Range(100, 1'000'000);

static void LruGetOperations(benchmark::State& state) {
    const std::size_t cache_size = state.range(0);
    UserLruCache cache(cache_size);
    PrepareCache(cache, cache_size);
    RunGetOperations(state, cache);
}
BENCHMARK(LruGetOperations)->RangeMultiplier(10)->Range(100, 1'000'000);

static void LruInsertOperations(benchmark::State& state) {
    const std::size_t cache_size = state.range(0);
    UserLruCache cache(cache_size);
    PrepareCache(cache, cache_size);
    RunInsertOperations(state, cache);
}
BENCHMARK(LruInsertOperations)->RangeMultiplier(10)->Range(100, 1'000'000);

// ExpirableContainer's benchmarks
static void ExpirableFindInsertMix(benchmark::State& state) {
    engine::RunStandalone([&] {
        const std::size_t cache_size = state.range(0);
        UserExpirableCache cache(cache_size, std::chrono::seconds(5));
        PrepareCache(cache, cache_size);
        RunFindInsertMix(state, cache);
    });
}
BENCHMARK(ExpirableFindInsertMix)->RangeMultiplier(10)->Range(10, 1'000'000);

static void ExpirableGetOperations(benchmark::State& state) {
    engine::RunStandalone([&] {
        const std::size_t cache_size = state.range(0);
        UserExpirableCache cache(cache_size, std::chrono::minutes(10));
        PrepareCache(cache, cache_size);
        RunGetOperations(state, cache);
    });
}
BENCHMARK(ExpirableGetOperations)->RangeMultiplier(10)->Range(100, 1'000'000);

static void ExpirableInsertOperations(benchmark::State& state) {
    engine::RunStandalone([&] {
        const std::size_t cache_size = state.range(0);
        UserExpirableCache cache(cache_size, std::chrono::minutes(10));
        PrepareCache(cache, cache_size);
        RunInsertOperations(state, cache);
    });
}
BENCHMARK(ExpirableInsertOperations)->RangeMultiplier(10)->Range(100, 1'000'000);

}  // namespace benchmarks

USERVER_NAMESPACE_END
