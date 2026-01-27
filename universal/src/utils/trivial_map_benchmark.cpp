#include <userver/utils/trivial_map.hpp>

#include <mutex>
#include <optional>
#include <unordered_map>

#include <benchmark/benchmark.h>

#include <utils/gbench_auxiliary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// We use this instead of Launder to cast the input to std::string_view
// and hide the string size from the optimizer.
std::string_view MyLaunder(std::string_view value) { return Launder(value); }

constexpr utils::TrivialBiMap kSmallTrivialBiMap = [](auto selector) {
    return selector().Case("hello", 1).Case("world", 2).Case("z", 42);
};

const auto kSmallUnorderedMapping = std::unordered_map<std::string_view, int>{
    {"hello", 1},
    {"world", 2},
    {"z", 42},
};

constexpr utils::TrivialBiMap kMediumTrivialBiMap = [](auto selector) {
    return selector()
        .Case("hello", 1)
        .Case("world", 2)
        .Case("a", 3)
        .Case("b", 4)
        .Case("c", 5)
        .Case("d", 6)
        .Case("e", 7)
        .Case("f", 8)
        .Case("z", 42);
};

const auto kMediumUnorderedMapping = std::unordered_map<std::string_view, int>{
    {"hello", 1},
    {"world", 2},
    {"a", 3},
    {"b", 4},
    {"c", 5},
    {"d", 6},
    {"e", 7},
    {"f", 8},
    {"z", 42},
};

constexpr utils::TrivialBiMap kHugeTrivialBiMap = [](auto selector) {
    return selector()
        .Case("aaaaaaaaaaaaaaaa_hello", 1)
        .Case("aaaaaaaaaaaaaaaa_world", 2)
        .Case("aaaaaaaaaaaaaaaa_a", 3)
        .Case("aaaaaaaaaaaaaaaa_b", 4)
        .Case("aaaaaaaaaaaaaaaa_c", 5)
        .Case("aaaaaaaaaaaaaaaa_d", 6)
        .Case("aaaaaaaaaaaaaaaa_e", 7)
        .Case("aaaaaaaaaaaaaaaa_f", 8)
        .Case("aaaaaaaaaaaaaaaa_f1", 8)
        .Case("aaaaaaaaaaaaaaaa_f2", 8)
        .Case("aaaaaaaaaaaaaaaa_f3", 8)
        .Case("aaaaaaaaaaaaaaaa_f4", 8)
        .Case("aaaaaaaaaaaaaaaa_f5", 8)
        .Case("aaaaaaaaaaaaaaaa_f6", 8)
        .Case("aaaaaaaaaaaaaaaa_f7", 8)
        .Case("aaaaaaaaaaaaaaaa_f8", 8)
        .Case("aaaaaaaaaaaaaaaa_f9", 8)
        .Case("aaaaaaaaaaaaaaaa_z", 42)
        .Case("aaaaaaaaaaaaaaaa_z1", 42)
        .Case("aaaaaaaaaaaaaaaa_z2", 42)
        .Case("aaaaaaaaaaaaaaaa_z3", 42)
        .Case("aaaaaaaaaaaaaaaa_z4", 42)
        .Case("aaaaaaaaaaaaaaaa_z5", 42)
        .Case("aaaaaaaaaaaaaaaa_z6", 42)
        .Case("aaaaaaaaaaaaaaaa_z7", 42)
        .Case("aaaaaaaaaaaaaaaa_z8", 42)
        .Case("aaaaaaaaaaaaaaaa_z9", 42)
        .Case("aaaaaaaaaaaaaaaa_x", 42)
        .Case("aaaaaaaaaaaaaaaa_x1", 42)
        .Case("aaaaaaaaaaaaaaaa_x2", 42)
        .Case("aaaaaaaaaaaaaaaa_x3", 42)
        .Case("aaaaaaaaaaaaaaaa_x4", 42)
        .Case("aaaaaaaaaaaaaaaa_x5", 42)
        .Case("aaaaaaaaaaaaaaaa_x6", 42)
        .Case("aaaaaaaaaaaaaaaa_x7", 42)
        .Case("aaaaaaaaaaaaaaaa_x8", 42)
        .Case("aaaaaaaaaaaaaaaa_x9", 42);
};

constexpr utils::StringLiteral kHugeTrivialBiMapKeys[] = {
    "aaaaaaaaaaaaaaaa_hello", "aaaaaaaaaaaaaaaa_world", "aaaaaaaaaaaaaaaa_a",  "aaaaaaaaaaaaaaaa_b",
    "aaaaaaaaaaaaaaaa_c",     "aaaaaaaaaaaaaaaa_d",     "aaaaaaaaaaaaaaaa_e",  "aaaaaaaaaaaaaaaa_f",
    "aaaaaaaaaaaaaaaa_f1",    "aaaaaaaaaaaaaaaa_f2",    "aaaaaaaaaaaaaaaa_f3", "aaaaaaaaaaaaaaaa_f4",
    "aaaaaaaaaaaaaaaa_f5",    "aaaaaaaaaaaaaaaa_f6",    "aaaaaaaaaaaaaaaa_f7", "aaaaaaaaaaaaaaaa_f8",
    "aaaaaaaaaaaaaaaa_f9",    "aaaaaaaaaaaaaaaa_z",     "aaaaaaaaaaaaaaaa_z1", "aaaaaaaaaaaaaaaa_z2",
    "aaaaaaaaaaaaaaaa_z3",    "aaaaaaaaaaaaaaaa_z4",    "aaaaaaaaaaaaaaaa_z5", "aaaaaaaaaaaaaaaa_z6",
    "aaaaaaaaaaaaaaaa_z7",    "aaaaaaaaaaaaaaaa_z8",    "aaaaaaaaaaaaaaaa_z9", "aaaaaaaaaaaaaaaa_x",
    "aaaaaaaaaaaaaaaa_x1",    "aaaaaaaaaaaaaaaa_x2",    "aaaaaaaaaaaaaaaa_x3", "aaaaaaaaaaaaaaaa_x4",
    "aaaaaaaaaaaaaaaa_x5",    "aaaaaaaaaaaaaaaa_x6",    "aaaaaaaaaaaaaaaa_x7", "aaaaaaaaaaaaaaaa_x8",
    "aaaaaaaaaaaaaaaa_x9",
};

constexpr int kHugeTrivialBiMapValues[] = {
    1,  2,  3,  4,  5,  6,  7,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
};

constexpr auto kHugeTrivialBiMapAlt = utils::MakeTrivialBiMap<kHugeTrivialBiMapKeys, kHugeTrivialBiMapValues>();

const auto kHugeUnorderedMapping = std::unordered_map<std::string_view, int>{
    {"aaaaaaaaaaaaaaaa_hello", 1}, {"aaaaaaaaaaaaaaaa_world", 2}, {"aaaaaaaaaaaaaaaa_a", 3},
    {"aaaaaaaaaaaaaaaa_b", 4},     {"aaaaaaaaaaaaaaaa_c", 5},     {"aaaaaaaaaaaaaaaa_d", 6},
    {"aaaaaaaaaaaaaaaa_e", 7},     {"aaaaaaaaaaaaaaaa_f", 8},     {"aaaaaaaaaaaaaaaa_f1", 8},
    {"aaaaaaaaaaaaaaaa_f2", 8},    {"aaaaaaaaaaaaaaaa_f3", 8},    {"aaaaaaaaaaaaaaaa_f4", 8},
    {"aaaaaaaaaaaaaaaa_f5", 8},    {"aaaaaaaaaaaaaaaa_f6", 8},    {"aaaaaaaaaaaaaaaa_f7", 8},
    {"aaaaaaaaaaaaaaaa_f8", 8},    {"aaaaaaaaaaaaaaaa_f9", 8},    {"aaaaaaaaaaaaaaaa_z", 42},
    {"aaaaaaaaaaaaaaaa_z1", 42},   {"aaaaaaaaaaaaaaaa_z2", 42},   {"aaaaaaaaaaaaaaaa_z3", 42},
    {"aaaaaaaaaaaaaaaa_z4", 42},   {"aaaaaaaaaaaaaaaa_z5", 42},   {"aaaaaaaaaaaaaaaa_z6", 42},
    {"aaaaaaaaaaaaaaaa_z7", 42},   {"aaaaaaaaaaaaaaaa_z8", 42},   {"aaaaaaaaaaaaaaaa_z9", 42},
    {"aaaaaaaaaaaaaaaa_x", 42},    {"aaaaaaaaaaaaaaaa_x1", 42},   {"aaaaaaaaaaaaaaaa_x2", 42},
    {"aaaaaaaaaaaaaaaa_x3", 42},   {"aaaaaaaaaaaaaaaa_x4", 42},   {"aaaaaaaaaaaaaaaa_x5", 42},
    {"aaaaaaaaaaaaaaaa_x6", 42},   {"aaaaaaaaaaaaaaaa_x7", 42},   {"aaaaaaaaaaaaaaaa_x8", 42},
    {"aaaaaaaaaaaaaaaa_x9", 42},
};

enum class Enum1 {
    kC1,
    kC2,
    kC3,
    kC4,
    kC5,
    kC6,
    kC7,
    kC8,
    kC9,
    kC10,
    kC11,
    kC12,
    kC13,
    kC14,
    kC15,
    kC16,
};

enum class Enum2 {
    kC1,
    kC2,
    kC3,
    kC4,
    kC5,
    kC6,
    kC7,
    kC8,
    kC9,
    kC10,
    kC11,
    kC12,
    kC13,
    kC14,
    kC15,
    kC16,
};

constexpr utils::TrivialBiMap kEnumTrivialBiMap = [](auto selector) {
    return selector()
        .Case(Enum1::kC1, Enum2::kC10)
        .Case(Enum1::kC2, Enum2::kC14)
        .Case(Enum1::kC3, Enum2::kC2)
        .Case(Enum1::kC4, Enum2::kC1)
        .Case(Enum1::kC5, Enum2::kC16)
        .Case(Enum1::kC6, Enum2::kC9)
        .Case(Enum1::kC7, Enum2::kC5)
        .Case(Enum1::kC8, Enum2::kC7)
        .Case(Enum1::kC9, Enum2::kC4)
        .Case(Enum1::kC10, Enum2::kC3)
        .Case(Enum1::kC11, Enum2::kC11)
        .Case(Enum1::kC12, Enum2::kC6)
        .Case(Enum1::kC13, Enum2::kC12)
        .Case(Enum1::kC14, Enum2::kC15)
        .Case(Enum1::kC15, Enum2::kC8)
        .Case(Enum1::kC16, Enum2::kC13);
};

std::optional<Enum1> Enum1From2Switch(Enum2 value) {
    switch (value) {
        case Enum2::kC10:
            return Enum1::kC1;
        case Enum2::kC14:
            return Enum1::kC2;
        case Enum2::kC2:
            return Enum1::kC3;
        case Enum2::kC1:
            return Enum1::kC4;
        case Enum2::kC16:
            return Enum1::kC5;
        case Enum2::kC9:
            return Enum1::kC6;
        case Enum2::kC5:
            return Enum1::kC7;
        case Enum2::kC7:
            return Enum1::kC8;
        case Enum2::kC4:
            return Enum1::kC9;
        case Enum2::kC3:
            return Enum1::kC10;
        case Enum2::kC11:
            return Enum1::kC11;
        case Enum2::kC6:
            return Enum1::kC12;
        case Enum2::kC12:
            return Enum1::kC13;
        case Enum2::kC15:
            return Enum1::kC14;
        case Enum2::kC8:
            return Enum1::kC15;
        case Enum2::kC13:
            return Enum1::kC16;
        default:
            return std::nullopt;
    }
}

}  // namespace

void MappingSmallTrivialBiMap(benchmark::State& state) {
    auto hello = MyLaunder("hello");
    auto world = MyLaunder("world");
    auto a = MyLaunder("a");
    auto b = MyLaunder("b");
    auto c = MyLaunder("c");

    auto d = MyLaunder("d");
    auto e = MyLaunder("e");
    auto f = MyLaunder("f");
    auto z = MyLaunder("z");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(hello));
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(world));
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(a));
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(b));
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(c));

        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(d));
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(e));
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(f));
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(z));
        benchmark::DoNotOptimize(kSmallTrivialBiMap.TryFind(z));
    }
}
BENCHMARK(MappingSmallTrivialBiMap);

void MappingSmallUnordered(benchmark::State& state) {
    auto hello = MyLaunder("hello");
    auto world = MyLaunder("world");
    auto a = MyLaunder("a");
    auto b = MyLaunder("b");
    auto c = MyLaunder("c");

    auto d = MyLaunder("d");
    auto e = MyLaunder("e");
    auto f = MyLaunder("f");
    auto z = MyLaunder("z");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(hello));
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(world));
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(a));
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(b));
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(c));

        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(d));
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(e));
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(f));
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(z));
        benchmark::DoNotOptimize(kSmallUnorderedMapping.find(z));
    }
}
BENCHMARK(MappingSmallUnordered);

void MappingMediumTrivialBiMap(benchmark::State& state) {
    auto hello = MyLaunder("hello");
    auto world = MyLaunder("world");
    auto a = MyLaunder("a");
    auto b = MyLaunder("b");
    auto c = MyLaunder("c");

    auto d = MyLaunder("d");
    auto e = MyLaunder("e");
    auto f = MyLaunder("f");
    auto z = MyLaunder("z");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(hello));
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(world));
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(a));
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(b));
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(c));

        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(d));
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(e));
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(f));
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(z));
        benchmark::DoNotOptimize(kMediumTrivialBiMap.TryFind(z));
    }
}
BENCHMARK(MappingMediumTrivialBiMap);

void MappingMediumUnordered(benchmark::State& state) {
    auto hello = MyLaunder("hello");
    auto world = MyLaunder("world");
    auto a = MyLaunder("a");
    auto b = MyLaunder("b");
    auto c = MyLaunder("c");

    auto d = MyLaunder("d");
    auto e = MyLaunder("e");
    auto f = MyLaunder("f");
    auto z = MyLaunder("z");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(hello));
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(world));
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(a));
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(b));
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(c));

        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(d));
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(e));
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(f));
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(z));
        benchmark::DoNotOptimize(kMediumUnorderedMapping.find(z));
    }
}
BENCHMARK(MappingMediumUnordered);

void MappingHugeTrivialBiMap(benchmark::State& state) {
    auto hello = MyLaunder("aaaaaaaaaaaaaaaa_hello");
    auto world = MyLaunder("aaaaaaaaaaaaaaaa_world");
    auto a = MyLaunder("aaaaaaaaaaaaaaaa_a");
    auto b = MyLaunder("aaaaaaaaaaaaaaaa_b");
    auto c = MyLaunder("aaaaaaaaaaaaaaaa_c");

    auto d = MyLaunder("aaaaaaaaaaaaaaaa_d");
    auto e = MyLaunder("aaaaaaaaaaaaaaaa_e");
    auto f9 = MyLaunder("aaaaaaaaaaaaaaaa_f9");
    auto z = MyLaunder("aaaaaaaaaaaaaaaa_z");
    auto z9 = MyLaunder("aaaaaaaaaaaaaaaa_z9");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(hello));
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(world));
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(a));
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(b));
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(c));

        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(d));
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(e));
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(f9));
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(z));
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(z9));
    }
}
BENCHMARK(MappingHugeTrivialBiMap);

void MappingHugeTrivialBiMapZip(benchmark::State& state) {
    auto hello = MyLaunder("aaaaaaaaaaaaaaaa_hello");
    auto world = MyLaunder("aaaaaaaaaaaaaaaa_world");
    auto a = MyLaunder("aaaaaaaaaaaaaaaa_a");
    auto b = MyLaunder("aaaaaaaaaaaaaaaa_b");
    auto c = MyLaunder("aaaaaaaaaaaaaaaa_c");

    auto d = MyLaunder("aaaaaaaaaaaaaaaa_d");
    auto e = MyLaunder("aaaaaaaaaaaaaaaa_e");
    auto f9 = MyLaunder("aaaaaaaaaaaaaaaa_f9");
    auto z = MyLaunder("aaaaaaaaaaaaaaaa_z");
    auto z9 = MyLaunder("aaaaaaaaaaaaaaaa_z9");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(hello));
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(world));
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(a));
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(b));
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(c));

        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(d));
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(e));
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(f9));
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(z));
        benchmark::DoNotOptimize(kHugeTrivialBiMapAlt.TryFind(z9));
    }
}
BENCHMARK(MappingHugeTrivialBiMapZip);

void MappingHugeUnordered(benchmark::State& state) {
    auto hello = MyLaunder("aaaaaaaaaaaaaaaa_hello");
    auto world = MyLaunder("aaaaaaaaaaaaaaaa_world");
    auto a = MyLaunder("aaaaaaaaaaaaaaaa_a");
    auto b = MyLaunder("aaaaaaaaaaaaaaaa_b");
    auto c = MyLaunder("aaaaaaaaaaaaaaaa_c");

    auto d = MyLaunder("aaaaaaaaaaaaaaaa_d");
    auto e = MyLaunder("aaaaaaaaaaaaaaaa_e");
    auto f9 = MyLaunder("aaaaaaaaaaaaaaaa_f9");
    auto z = MyLaunder("aaaaaaaaaaaaaaaa_z");
    auto z9 = MyLaunder("aaaaaaaaaaaaaaaa_z9");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(hello));
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(world));
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(a));
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(b));
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(c));

        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(d));
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(e));
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(f9));
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(z));
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(z9));
    }
}
BENCHMARK(MappingHugeUnordered);

void MappingHugeTrivialBiMapLast(benchmark::State& state) {
    auto z9 = MyLaunder("aaaaaaaaaaaaaaaa_z9");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kHugeTrivialBiMap.TryFind(z9));
    }
}
BENCHMARK(MappingHugeTrivialBiMapLast);

void MappingHugeUnorderedLast(benchmark::State& state) {
    auto z9 = MyLaunder("aaaaaaaaaaaaaaaa_z9");

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kHugeUnorderedMapping.find(z9));
    }
}
BENCHMARK(MappingHugeUnorderedLast);

void MappingEnumsTrivialBiMap(benchmark::State& state) {
    const auto enum2 = Launder(Enum2::kC7);

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(kEnumTrivialBiMap.TryFind(enum2));
    }
}
BENCHMARK(MappingEnumsTrivialBiMap);

void MappingEnumsSwitch(benchmark::State& state) {
    const auto enum2 = Launder(Enum2::kC7);

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(Enum1From2Switch(enum2));
    }
}
BENCHMARK(MappingEnumsSwitch);

USERVER_NAMESPACE_END
