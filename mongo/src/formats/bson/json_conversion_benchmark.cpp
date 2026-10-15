#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <userver/formats/bson/binary.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

enum class Shape { kWide, kDeep, kNested };
enum class BsonInput { kParsed, kUnparsed };

formats::json::Value MakeFixture(Shape shape, std::int64_t size) {
    namespace json = formats::json;
    json::ValueBuilder result(formats::common::Type::kObject);
    constexpr std::size_t kPayloadSize = 1024;
    if (shape == Shape::kDeep) {
        result["payload"] = std::string(kPayloadSize, 'x');
        for (std::int64_t i = 0; i < size; ++i) {
            json::ValueBuilder parent;
            parent["child"] = std::move(result);
            result = std::move(parent);
        }
    } else {
        for (std::int64_t i = 0; i < size; ++i) {
            const auto key = "field_" + std::to_string(i);
            if (shape == Shape::kWide) {
                result.EmplaceNocheck(key, i);
            } else {
                json::ValueBuilder record;
                record["id"] = i;
                record["price"]["amount"] = 12.5;
                record["price"]["currency"] = "EUR";
                record["metadata"]["payload"] = std::string(kPayloadSize, 'x');
                record["metadata"]["tags"] = json::MakeArray("one", "two", nullptr);
                result.EmplaceNocheck(key, std::move(record));
            }
        }
    }
    return result.ExtractValue();
}

void JsonToBson(benchmark::State& state, Shape shape) {
    const auto input = MakeFixture(shape, state.range(0));
    const auto bytes = static_cast<std::int64_t>(formats::json::ToString(input).size());
    for (auto _ : state) {
        auto output = input.ConvertTo<formats::bson::Value>();
        benchmark::DoNotOptimize(output);
    }
    state.SetBytesProcessed(state.iterations() * bytes);
}

void BsonToJson(benchmark::State& state, Shape shape, BsonInput input_kind) {
    const auto fixture = MakeFixture(shape, state.range(0));
    const auto binary = formats::bson::ToBinaryString(fixture.ConvertTo<formats::bson::Value>());
    const auto parsed = formats::bson::FromBinaryString(binary.GetView());
    benchmark::DoNotOptimize(parsed.ConvertTo<formats::json::Value>());
    for (auto _ : state) {
        const auto
            input = input_kind == BsonInput::kParsed ? parsed : formats::bson::FromBinaryString(binary.GetView());
        auto output = input.ConvertTo<formats::json::Value>();
        benchmark::DoNotOptimize(output);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(binary.GetView().size()));
}

BENCHMARK_CAPTURE(JsonToBson, Wide, Shape::kWide)->Arg(8)->Arg(128)->Arg(1024)->Arg(4096);
BENCHMARK_CAPTURE(JsonToBson, Deep, Shape::kDeep)->Arg(1)->Arg(8)->Arg(32)->Arg(64);
BENCHMARK_CAPTURE(JsonToBson, Nested, Shape::kNested)->Arg(1)->Arg(8)->Arg(32)->Arg(128);
BENCHMARK_CAPTURE(BsonToJson, WideParsed, Shape::kWide, BsonInput::kParsed)->Arg(8)->Arg(128)->Arg(1024)->Arg(4096);
BENCHMARK_CAPTURE(BsonToJson, WideUnparsed, Shape::kWide, BsonInput::kUnparsed)->Arg(8)->Arg(128)->Arg(1024)->Arg(4096);
BENCHMARK_CAPTURE(BsonToJson, DeepParsed, Shape::kDeep, BsonInput::kParsed)->Arg(1)->Arg(8)->Arg(32)->Arg(64);
BENCHMARK_CAPTURE(BsonToJson, DeepUnparsed, Shape::kDeep, BsonInput::kUnparsed)->Arg(1)->Arg(8)->Arg(32)->Arg(64);
BENCHMARK_CAPTURE(BsonToJson, NestedParsed, Shape::kNested, BsonInput::kParsed)->Arg(1)->Arg(8)->Arg(32)->Arg(128);
BENCHMARK_CAPTURE(BsonToJson, NestedUnparsed, Shape::kNested, BsonInput::kUnparsed)->Arg(1)->Arg(8)->Arg(32)->Arg(128);

}  // namespace

USERVER_NAMESPACE_END
