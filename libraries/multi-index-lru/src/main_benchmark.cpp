#include <userver/multi_index_lru/container.hpp>
#include <userver/utils/rand.hpp>

#include <random>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>
#include <boost/multi_index/member.hpp>

USERVER_NAMESPACE_BEGIN

namespace benchmarks {

const size_t OPERATIONS_NUMBER = 100000;
const int MAX_ID_SIZE = 50000;

struct id_tag {};
struct email_tag {};
struct name_tag {};

struct User {
    int id;
    std::string email;
    std::string name;

    bool operator==(const User& other) const { return id == other.id && email == other.email && name == other.name; }
};

namespace {
User generateUser() {
    std::string email = "email" + std::to_string(utils::RandRange<int>(0, MAX_ID_SIZE));
    std::string name = "name" + std::to_string(utils::RandRange<int>(0, MAX_ID_SIZE));
    return User{utils::RandRange<int>(0, MAX_ID_SIZE), email, name};
}

int generateId() { return utils::RandRange<int>(0, MAX_ID_SIZE); }

std::string generateName() { return "name" + std::to_string(utils::RandRange<int>(0, MAX_ID_SIZE)); }

std::string generateEmail() { return "email" + std::to_string(utils::RandRange<int>(0, MAX_ID_SIZE)); }
} // namespace

using UserCache = multi_index_lru::Container<
    User,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::tag<id_tag>, boost::multi_index::member<User, int, &User::id>>,
        boost::multi_index::
            ordered_unique<boost::multi_index::tag<email_tag>, boost::multi_index::member<User, std::string, &User::email>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<name_tag>,
            boost::multi_index::member<User, std::string, &User::name>>>>;

void LruFindEmplaceMix(benchmark::State& state) {
    const size_t size = state.range(0);

    UserCache cache(size);
    for (size_t i = 0; i < size; ++i) {
        cache.emplace(generateUser());
    }

    size_t reading_operations_number = OPERATIONS_NUMBER * 4 / 5;
    size_t writing_operations_number = OPERATIONS_NUMBER / 5;

    std::vector<std::string> names, emails;
    std::vector<int> ids;
    std::vector<User> users;

    for (size_t i = 0; i < reading_operations_number; ++i) {
        names.push_back(generateName());
        emails.push_back(generateEmail());
        ids.push_back(generateId());
    }

    for (size_t i = 0; i < writing_operations_number; ++i) {
        users.push_back(generateUser());
    }

    for ([[maybe_unused]] auto _ : state) {
        for (size_t i = 0; i < reading_operations_number; ++i) {
            cache.find<name_tag, std::string>(names[i]);
            cache.find<email_tag, std::string>(emails[i]);
            cache.find<id_tag, int>(ids[i]);
        }

        for (size_t i = 0; i < writing_operations_number; ++i) {
            cache.emplace(users[i]);
        }
    }
}

BENCHMARK(LruFindEmplaceMix)->RangeMultiplier(10)->Range(100, 1000000);

static void prepareCache(UserCache& cache, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        cache.emplace(generateUser());
    }
}

static void GetOperations(::benchmark::State& state) {
    const size_t cache_size = state.range(0);
    const size_t operations_count = OPERATIONS_NUMBER;

    UserCache cache(cache_size);
    prepareCache(cache, cache_size);

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::string> names, emails;
        std::vector<int> ids;
        for (size_t i = 0; i < operations_count; ++i) {
            names.push_back(generateName());
            emails.push_back(generateEmail());
            ids.push_back(generateId());
        }
        state.ResumeTiming();

        for (size_t i = 0; i < operations_count; ++i) {
            ::benchmark::DoNotOptimize(cache.find<name_tag, std::string>(names[i]));
            ::benchmark::DoNotOptimize(cache.find<email_tag, std::string>(emails[i]));
            ::benchmark::DoNotOptimize(cache.find<id_tag, int>(ids[i]));
        }
    }

    state.SetItemsProcessed(state.iterations() * operations_count * 3);
    state.SetComplexityN(cache_size);
}

BENCHMARK(GetOperations)->RangeMultiplier(10)->Range(100, 1000000);

static void EmplaceOperations(::benchmark::State& state) {
    const size_t cache_size = state.range(0);
    const size_t operations_count = OPERATIONS_NUMBER;

    UserCache cache(cache_size);
    prepareCache(cache, cache_size);

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<User> users;
        for (size_t i = 0; i < operations_count; ++i) {
            users.push_back(generateUser());
        }
        state.ResumeTiming();

        for (size_t i = 0; i < operations_count; ++i) {
            cache.emplace(users[i]);
        }
    }

    state.SetItemsProcessed(state.iterations() * operations_count);
    state.SetComplexityN(cache_size);
}

BENCHMARK(EmplaceOperations)->RangeMultiplier(10)->Range(100, 1000000);

}  // namespace benchmarks

USERVER_NAMESPACE_END