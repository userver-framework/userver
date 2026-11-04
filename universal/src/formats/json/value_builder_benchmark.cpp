#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::size_t kSmallArraySize = 8;
constexpr std::size_t kMediumArraySize = 128;
constexpr std::size_t kLargeArraySize = 1024;
constexpr std::string_view kStringValue = "value_for_direct_string_push_back";

struct SerializableInteger {
    std::int64_t value;
};

formats::json::Value Serialize(const SerializableInteger& value, formats::serialize::To<formats::json::Value>) {
    return formats::json::ValueBuilder{value.value}.ExtractValue();
}

enum class SerializedInsertion { kAssignment, kEmplace, kPushBack };

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

std::vector<std::string> MakeKeys(std::size_t size) {
    std::vector<std::string> keys;
    keys.reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        keys.push_back(std::to_string(index));
    }
    return keys;
}

template <formats::json::Type ContainerType = formats::json::Type::kArray, typename Inserter>
void BenchmarkBorrowedValues(benchmark::State& state, const formats::json::Value& source, Inserter insert) {
    const auto size = source.GetSize();
    const auto keys = MakeKeys(size);
    for ([[maybe_unused]] auto _ : state) {
        formats::json::ValueBuilder result{ContainerType};
        result.Reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            const auto value = source[index];
            insert(result, value, keys[index]);
        }
        benchmark::DoNotOptimize(result.ExtractValue());
    }
    state.SetItemsProcessed(state.iterations() * size);
}

template <typename Value, formats::json::Type ContainerType = formats::json::Type::kArray, typename Inserter>
void BenchmarkGeneratedScalars(benchmark::State& state, Inserter insert) {
    const auto size = static_cast<std::size_t>(state.range(0));
    const auto keys = MakeKeys(size);
    for ([[maybe_unused]] auto _ : state) {
        formats::json::ValueBuilder result{ContainerType};
        result.Reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            insert(result, static_cast<Value>(index), keys[index]);
        }
        benchmark::DoNotOptimize(result.ExtractValue());
    }
    state.SetItemsProcessed(state.iterations() * size);
}

template <SerializedInsertion Insertion, bool ReuseSerializedRoot>
void BenchmarkSerializedIntegers(benchmark::State& state) {
    constexpr auto kContainer =
        Insertion == SerializedInsertion::kPushBack ? formats::json::Type::kArray : formats::json::Type::kObject;
    BenchmarkGeneratedScalars<
        std::int64_t,
        kContainer>(state, [](auto& result, std::int64_t value, const std::string& key) {
        const auto make_item = [value] {
            const SerializableInteger serialized{value};
            if constexpr (ReuseSerializedRoot) {
                return serialized;
            } else {
                return formats::json::ValueBuilder{Serialize(serialized, formats::serialize::To<formats::json::Value>{})
                };
            }
        };
        if constexpr (Insertion == SerializedInsertion::kAssignment) {
            result[key] = make_item();
        } else if constexpr (Insertion == SerializedInsertion::kEmplace) {
            result.EmplaceNocheck(key, make_item());
        } else {
            result.PushBack(make_item());
        }
    });
}

void AssignSerializedIntegersViaValue(benchmark::State& state) {
    BenchmarkSerializedIntegers<SerializedInsertion::kAssignment, false>(state);
}

void AssignSerializedIntegersDirectly(benchmark::State& state) {
    BenchmarkSerializedIntegers<SerializedInsertion::kAssignment, true>(state);
}

void EmplaceNocheckSerializedIntegersViaValue(benchmark::State& state) {
    BenchmarkSerializedIntegers<SerializedInsertion::kEmplace, false>(state);
}

void EmplaceNocheckSerializedIntegersDirectly(benchmark::State& state) {
    BenchmarkSerializedIntegers<SerializedInsertion::kEmplace, true>(state);
}

void PushBackSerializedIntegersViaValue(benchmark::State& state) {
    BenchmarkSerializedIntegers<SerializedInsertion::kPushBack, false>(state);
}

void PushBackSerializedIntegersDirectly(benchmark::State& state) {
    BenchmarkSerializedIntegers<SerializedInsertion::kPushBack, true>(state);
}

template <formats::json::Type ContainerType = formats::json::Type::kArray, typename Inserter>
void BenchmarkBorrowedStrings(benchmark::State& state, Inserter insert) {
    const auto values = MakeStrings(state.range(0));
    const auto keys = MakeKeys(values.size());
    for ([[maybe_unused]] auto _ : state) {
        formats::json::ValueBuilder result{ContainerType};
        result.Reserve(values.size());
        for (std::size_t index = 0; index < values.size(); ++index) {
            const auto& value = values[index];
            insert(result, value, keys[index]);
        }
        benchmark::DoNotOptimize(result.ExtractValue());
    }
    state.SetItemsProcessed(state.iterations() * values.size());
}

template <formats::json::Type ContainerType = formats::json::Type::kArray, typename Inserter>
void BenchmarkGeneratedNulls(benchmark::State& state, Inserter insert) {
    const auto size = static_cast<std::size_t>(state.range(0));
    const auto keys = MakeKeys(size);
    for ([[maybe_unused]] auto _ : state) {
        formats::json::ValueBuilder result{ContainerType};
        result.Reserve(size);
        for (std::size_t index = 0; index < size; ++index) {
            insert(result, keys[index]);
        }
        benchmark::DoNotOptimize(result.ExtractValue());
    }
    state.SetItemsProcessed(state.iterations() * size);
}

void PushBackGeneratedIntegersViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedScalars<std::int64_t>(state, [](auto& result, std::int64_t value, std::string_view) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackGeneratedIntegersDirectly(benchmark::State& state) {
    BenchmarkGeneratedScalars<std::int64_t>(state, [](auto& result, std::int64_t value, std::string_view) {
        result.PushBack(value);
    });
}

void PushBackGeneratedDoublesViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedScalars<double>(state, [](auto& result, double value, std::string_view) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackGeneratedDoublesDirectly(benchmark::State& state) {
    BenchmarkGeneratedScalars<double>(state, [](auto& result, double value, std::string_view) {
        result.PushBack(value);
    });
}

void PushBackBorrowedStringsViaBuilder(benchmark::State& state) {
    BenchmarkBorrowedStrings(state, [](auto& result, const auto& value, std::string_view) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackBorrowedStringsDirectly(benchmark::State& state) {
    BenchmarkBorrowedStrings(state, [](auto& result, const auto& value, std::string_view) { result.PushBack(value); });
}

void PushBackGeneratedNullsViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedNulls(state, [](auto& result, std::string_view) {
        result.PushBack(formats::json::ValueBuilder{});
    });
}

void PushBackGeneratedNullsDirectly(benchmark::State& state) {
    BenchmarkGeneratedNulls(state, [](auto& result, std::string_view) { result.PushBack(nullptr); });
}

void PushBackBorrowedScalarsViaBuilder(benchmark::State& state) {
    const auto source = MakeScalarArray(state.range(0));
    BenchmarkBorrowedValues(state, source, [](auto& result, const auto& value, std::string_view) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackBorrowedScalarsDirectly(benchmark::State& state) {
    const auto source = MakeScalarArray(state.range(0));
    BenchmarkBorrowedValues(state, source, [](auto& result, const auto& value, std::string_view) {
        result.PushBack(value);
    });
}

void PushBackBorrowedObjectsViaBuilder(benchmark::State& state) {
    const auto source = MakeObjectArray(state.range(0));
    BenchmarkBorrowedValues(state, source, [](auto& result, const auto& value, std::string_view) {
        result.PushBack(formats::json::ValueBuilder{value});
    });
}

void PushBackBorrowedObjectsDirectly(benchmark::State& state) {
    const auto source = MakeObjectArray(state.range(0));
    BenchmarkBorrowedValues(state, source, [](auto& result, const auto& value, std::string_view) {
        result.PushBack(value);
    });
}

void EmplaceNocheckGeneratedIntegersViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedScalars<
        std::int64_t,
        formats::json::Type::kObject>(state, [](auto& result, std::int64_t value, std::string_view key) {
        result.EmplaceNocheck(key, formats::json::ValueBuilder{value});
    });
}

void EmplaceNocheckGeneratedIntegersDirectly(benchmark::State& state) {
    BenchmarkGeneratedScalars<
        std::int64_t,
        formats::json::Type::kObject>(state, [](auto& result, std::int64_t value, std::string_view key) {
        result.EmplaceNocheck(key, value);
    });
}

void EmplaceNocheckGeneratedDoublesViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedScalars<
        double,
        formats::json::Type::kObject>(state, [](auto& result, double value, std::string_view key) {
        result.EmplaceNocheck(key, formats::json::ValueBuilder{value});
    });
}

void EmplaceNocheckGeneratedDoublesDirectly(benchmark::State& state) {
    BenchmarkGeneratedScalars<
        double,
        formats::json::Type::kObject>(state, [](auto& result, double value, std::string_view key) {
        result.EmplaceNocheck(key, value);
    });
}

void EmplaceNocheckBorrowedStringsViaBuilder(benchmark::State& state) {
    BenchmarkBorrowedStrings<
        formats::json::Type::kObject>(state, [](auto& result, const auto& value, std::string_view key) {
        result.EmplaceNocheck(key, formats::json::ValueBuilder{value});
    });
}

void EmplaceNocheckBorrowedStringsDirectly(benchmark::State& state) {
    BenchmarkBorrowedStrings<
        formats::json::Type::kObject>(state, [](auto& result, const auto& value, std::string_view key) {
        result.EmplaceNocheck(key, value);
    });
}

void EmplaceNocheckGeneratedNullsViaBuilder(benchmark::State& state) {
    BenchmarkGeneratedNulls<formats::json::Type::kObject>(state, [](auto& result, std::string_view key) {
        result.EmplaceNocheck(key, formats::json::ValueBuilder{});
    });
}

void EmplaceNocheckGeneratedNullsDirectly(benchmark::State& state) {
    BenchmarkGeneratedNulls<formats::json::Type::kObject>(state, [](auto& result, std::string_view key) {
        result.EmplaceNocheck(key, nullptr);
    });
}

void EmplaceNocheckBorrowedScalarsViaBuilder(benchmark::State& state) {
    const auto source = MakeScalarArray(state.range(0));
    BenchmarkBorrowedValues<
        formats::json::Type::kObject>(state, source, [](auto& result, const auto& value, std::string_view key) {
        result.EmplaceNocheck(key, formats::json::ValueBuilder{value});
    });
}

void EmplaceNocheckBorrowedScalarsDirectly(benchmark::State& state) {
    const auto source = MakeScalarArray(state.range(0));
    BenchmarkBorrowedValues<
        formats::json::Type::kObject>(state, source, [](auto& result, const auto& value, std::string_view key) {
        result.EmplaceNocheck(key, value);
    });
}

void EmplaceNocheckBorrowedObjectsViaBuilder(benchmark::State& state) {
    const auto source = MakeObjectArray(state.range(0));
    BenchmarkBorrowedValues<
        formats::json::Type::kObject>(state, source, [](auto& result, const auto& value, std::string_view key) {
        result.EmplaceNocheck(key, formats::json::ValueBuilder{value});
    });
}

void EmplaceNocheckBorrowedObjectsDirectly(benchmark::State& state) {
    const auto source = MakeObjectArray(state.range(0));
    BenchmarkBorrowedValues<
        formats::json::Type::kObject>(state, source, [](auto& result, const auto& value, std::string_view key) {
        result.EmplaceNocheck(key, value);
    });
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

BENCHMARK(EmplaceNocheckBorrowedScalarsViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckBorrowedScalarsDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckBorrowedObjectsViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckBorrowedObjectsDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckGeneratedIntegersViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckGeneratedIntegersDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckGeneratedDoublesViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckGeneratedDoublesDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckBorrowedStringsViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckBorrowedStringsDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckGeneratedNullsViaBuilder)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckGeneratedNullsDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);

BENCHMARK(AssignSerializedIntegersViaValue)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(AssignSerializedIntegersDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckSerializedIntegersViaValue)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(EmplaceNocheckSerializedIntegersDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackSerializedIntegersViaValue)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);
BENCHMARK(PushBackSerializedIntegersDirectly)->Arg(kSmallArraySize)->Arg(kMediumArraySize)->Arg(kLargeArraySize);

}  // namespace

USERVER_NAMESPACE_END
