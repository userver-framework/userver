#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::size_t kSmallArraySize = 8;
constexpr std::size_t kMediumArraySize = 128;
constexpr std::size_t kLargeArraySize = 1024;
constexpr std::string_view kStringValue = "value_for_direct_string_push_back";

formats::json::Value MakeScalarArray(std::size_t size) {
    formats::json::ValueBuilder source{formats::json::Type::kArray};
    source.Reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        source.PushBack(formats::json::ValueBuilder{index});
    }
    return source.ExtractValue();
}

formats::json::Value MakeObjectArray(std::size_t size) {
    formats::json::ValueBuilder source{formats::json::Type::kArray};
    source.Reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        formats::json::ValueBuilder item{formats::json::Type::kObject};
        item["index"] = index;
        item["payload"] = "value_" + std::to_string(index);
        source.PushBack(std::move(item));
    }
    return source.ExtractValue();
}

std::vector<std::string> MakeStrings(std::size_t size) {
    return std::vector<std::string>(size, std::string{kStringValue});
}

template <typename PushBack>
void BenchmarkBorrowedValues(benchmark::State& state, const formats::json::Value& source, PushBack push_back) {
    const auto size = source.GetSize();
    for ([[maybe_unused]] auto _ : state) {
        formats::json::ValueBuilder result{formats::json::Type::kArray};
        result.Reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            const auto value = source[index];
            push_back(result, value);
        }
        benchmark::DoNotOptimize(result.ExtractValue());
    }
    state.SetItemsProcessed(state.iterations() * size);
}

template <typename Value, typename PushBack>
void BenchmarkGeneratedScalars(benchmark::State& state, PushBack push_back) {
    const auto size = static_cast<std::size_t>(state.range(0));
    for ([[maybe_unused]] auto _ : state) {
        formats::json::ValueBuilder result{formats::json::Type::kArray};
        result.Reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            push_back(result, static_cast<Value>(index));
        }
        benchmark::DoNotOptimize(result.ExtractValue());
    }
    state.SetItemsProcessed(state.iterations() * size);
}

template <typename PushBack>
void BenchmarkBorrowedStrings(benchmark::State& state, PushBack push_back) {
    const auto values = MakeStrings(state.range(0));
    for ([[maybe_unused]] auto _ : state) {
        formats::json::ValueBuilder result{formats::json::Type::kArray};
        result.Reserve(values.size());
        for (const auto& value : values) {
            push_back(result, value);
        }
        benchmark::DoNotOptimize(result.ExtractValue());
    }
    state.SetItemsProcessed(state.iterations() * values.size());
}

template <typename PushBack>
void BenchmarkGeneratedNulls(benchmark::State& state, PushBack push_back) {
    const auto size = static_cast<std::size_t>(state.range(0));
    for ([[maybe_unused]] auto _ : state) {
        formats::json::ValueBuilder result{formats::json::Type::kArray};
        result.Reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            push_back(result);
        }
        benchmark::DoNotOptimize(result.ExtractValue());
    }
    state.SetItemsProcessed(state.iterations() * size);
}

void PushBackGeneratedIntegersViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedScalars<std::int64_t>(state, [](auto& result, std::int64_t value) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackGeneratedIntegersDirectly(benchmark::State& state) {
    BenchmarkGeneratedScalars<std::int64_t>(state, [](auto& result, std::int64_t value) { result.PushBack(value); });
}

void PushBackGeneratedDoublesViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedScalars<double>(state, [](auto& result, double value) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackGeneratedDoublesDirectly(benchmark::State& state) {
    BenchmarkGeneratedScalars<double>(state, [](auto& result, double value) { result.PushBack(value); });
}

void PushBackBorrowedStringsViaBuilder(benchmark::State& state) {
    BenchmarkBorrowedStrings(state, [](auto& result, const auto& value) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackBorrowedStringsDirectly(benchmark::State& state) {
    BenchmarkBorrowedStrings(state, [](auto& result, const auto& value) { result.PushBack(value); });
}

void PushBackGeneratedNullsViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedNulls(state, [](auto& result) { result.PushBack(formats::json::ValueBuilder{}); });
}

void PushBackGeneratedNullsDirectly(benchmark::State& state) {
    BenchmarkGeneratedNulls(state, [](auto& result) { result.PushBack(nullptr); });
}

void PushBackBorrowedScalarsViaBuilder(benchmark::State& state) {
    const auto source = MakeScalarArray(state.range(0));
    BenchmarkBorrowedValues(state, source, [](auto& result, const auto& value) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackBorrowedScalarsDirectly(benchmark::State& state) {
    const auto source = MakeScalarArray(state.range(0));
    BenchmarkBorrowedValues(state, source, [](auto& result, const auto& value) { result.PushBack(value); });
}

void PushBackBorrowedObjectsViaBuilder(benchmark::State& state) {
    const auto source = MakeObjectArray(state.range(0));
    BenchmarkBorrowedValues(state, source, [](auto& result, const auto& value) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackBorrowedObjectsDirectly(benchmark::State& state) {
    const auto source = MakeObjectArray(state.range(0));
    BenchmarkBorrowedValues(state, source, [](auto& result, const auto& value) { result.PushBack(value); });
}

BENCHMARK(PushBackBorrowedScalarsViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackBorrowedScalarsDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackBorrowedObjectsViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackBorrowedObjectsDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackGeneratedIntegersViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackGeneratedIntegersDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackGeneratedDoublesViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackGeneratedDoublesDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackBorrowedStringsViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackBorrowedStringsDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackGeneratedNullsViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackGeneratedNullsDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);

}  // namespace

USERVER_NAMESPACE_END
