#include <userver/multi-index-lru/container.hpp>
#include <userver/utils/rand.hpp>

#include <random>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>
#include <boost/multi_index/member.hpp>

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

namespace {
User GenerateUser() {
    std::string email = "email" + std::to_string(utils::RandRange<int>(0, kMaxIdSize));
    std::string name = "name" + std::to_string(utils::RandRange<int>(0, kMaxIdSize));
    return User{utils::RandRange<int>(0, kMaxIdSize), email, name};
}

int GenerateId() { return utils::RandRange<int>(0, kMaxIdSize); }

std::string GenerateName() { return "name" + std::to_string(utils::RandRange<int>(0, kMaxIdSize)); }

std::string GenerateEmail() { return "email" + std::to_string(utils::RandRange<int>(0, kMaxIdSize)); }
}  // namespace

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

void LruFindEmplaceMix(benchmark::State& state) {
    const std::size_t size = state.range(0);

    UserCache cache(size);
    for (std::size_t i = 0; i < size; ++i) {
        cache.emplace(GenerateUser());
    }

    std::size_t reading_kOperationsNumber = kOperationsNumber * 4 / 5;
    std::size_t writing_kOperationsNumber = kOperationsNumber / 5;

    std::vector<std::string> names, emails;
    std::vector<int> ids;
    std::vector<User> users;

    for (std::size_t i = 0; i < reading_kOperationsNumber; ++i) {
        names.push_back(GenerateName());
        emails.push_back(GenerateEmail());
        ids.push_back(GenerateId());
    }

    for (std::size_t i = 0; i < writing_kOperationsNumber; ++i) {
        users.push_back(GenerateUser());
    }

    for ([[maybe_unused]] auto _ : state) {
        for (std::size_t i = 0; i < reading_kOperationsNumber; ++i) {
            cache.find<NameTag, std::string>(names[i]);
            cache.find<EmailTag, std::string>(emails[i]);
            cache.find<IdTag, int>(ids[i]);
        }

        for (std::size_t i = 0; i < writing_kOperationsNumber; ++i) {
            cache.emplace(users[i]);
        }
    }
}

BENCHMARK(LruFindEmplaceMix)->RangeMultiplier(10)->Range(100, 1000000);

static void prepareCache(UserCache& cache, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        cache.emplace(GenerateUser());
    }
}

static void GetOperations(::benchmark::State& state) {
    const std::size_t cache_size = state.range(0);
    const std::size_t operations_count = kOperationsNumber;

    UserCache cache(cache_size);
    prepareCache(cache, cache_size);

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::string> names, emails;
        std::vector<int> ids;
        for (std::size_t i = 0; i < operations_count; ++i) {
            names.push_back(GenerateName());
            emails.push_back(GenerateEmail());
            ids.push_back(GenerateId());
        }
        state.ResumeTiming();

        for (std::size_t i = 0; i < operations_count; ++i) {
            ::benchmark::DoNotOptimize(cache.find<NameTag, std::string>(names[i]));
            ::benchmark::DoNotOptimize(cache.find<EmailTag, std::string>(emails[i]));
            ::benchmark::DoNotOptimize(cache.find<IdTag, int>(ids[i]));
        }
    }

    state.SetItemsProcessed(state.iterations() * operations_count * 3);
    state.SetComplexityN(cache_size);
}

BENCHMARK(GetOperations)->RangeMultiplier(10)->Range(100, 1000000);

static void EmplaceOperations(::benchmark::State& state) {
    const std::size_t cache_size = state.range(0);
    const std::size_t operations_count = kOperationsNumber;

    UserCache cache(cache_size);
    prepareCache(cache, cache_size);

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<User> users;
        for (std::size_t i = 0; i < operations_count; ++i) {
            users.push_back(GenerateUser());
        }
        state.ResumeTiming();

        for (std::size_t i = 0; i < operations_count; ++i) {
            cache.emplace(users[i]);
        }
    }

    state.SetItemsProcessed(state.iterations() * operations_count);
    state.SetComplexityN(cache_size);
}

BENCHMARK(EmplaceOperations)->RangeMultiplier(10)->Range(100, 1000000);

}  // namespace benchmarks

USERVER_NAMESPACE_END
