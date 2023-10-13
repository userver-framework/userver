#include <benchmark/benchmark.h>

#include <fmt/format.h>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/parser/parser.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string Build(size_t len) {
  std::string r = "[";
  for (size_t i = 0; i < len; i++) {
    if (i > 0) r += ',';
    r += '1';
  }
  r += ']';
  return r;
}

std::string BuildArray(size_t len) {
  std::string r = "[";
  for (size_t i = 0; i < len; i++) {
    if (i > 0) r += ',';
    r += Build(len);
  }
  r += ']';
  return r;
}

auto ParseDom(const formats::json::Value& value) {
  return value.As<std::vector<std::vector<int64_t>>>();
}

}  // namespace

void JsonParseArrayDom(benchmark::State& state) {
  const auto input = BuildArray(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    auto json = formats::json::FromString(input);
    const auto res = ParseDom(json);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(JsonParseArrayDom)->RangeMultiplier(4)->Range(1, 1024);

void JsonParseArraySax(benchmark::State& state) {
  const auto input = BuildArray(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    std::vector<std::vector<int64_t>> result{};
    using Int64Parser = formats::json::parser::Int64Parser;
    Int64Parser int_parser;
    using ArrayParser =
        formats::json::parser::ArrayParser<int64_t, Int64Parser>;

    ArrayParser array_parser(int_parser);
    formats::json::parser::ArrayParser<std::vector<int64_t>, ArrayParser>
        parser(array_parser);
    parser.Reset();

    formats::json::parser::ParserState state;
    state.PushParser(parser);
    state.ProcessInput(input);
  }
}
BENCHMARK(JsonParseArraySax)->RangeMultiplier(4)->Range(1, 1024);

std::string BuildObject(size_t level) {
  if (level == 0) {
    return R"({"k": 123, "v": 1.11, "s": "some string"})";
  }

  auto subobj = BuildObject(level - 1);

  return fmt::format(R"({{"one": {}, "two": {}, "three": "string"}})", subobj,
                     subobj);
}

void JsonParseValueDom(benchmark::State& state) {
  const auto input = BuildObject(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    const auto res = formats::json::FromString(input);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(JsonParseValueDom)->RangeMultiplier(2)->Range(1, 16);

void JsonParseValueSax(benchmark::State& state) {
  const auto input = BuildObject(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    const auto res = formats::json::parser::ParseToType<
        formats::json::Value, formats::json::parser::JsonValueParser>(input);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(JsonParseValueSax)->RangeMultiplier(2)->Range(1, 16);

namespace {

struct SomeValue final {
  std::size_t value;

  bool operator==(const SomeValue& other) const { return value == other.value; }
};

SomeValue Parse(const formats::json::Value& value,
                formats::parse::To<SomeValue>) {
  SomeValue result{};
  const std::string we = formats::json::ToString(value);
  result.value = value["value"].As<std::size_t>(0);

  return result;
}

formats::json::Value Serialize(const SomeValue& value,
                               formats::serialize::To<formats::json::Value>) {
  return formats::json::MakeObject("no_value", value.value);
}

struct ObjValue final {
  SomeValue value;

  bool operator==(const ObjValue& other) const { return value == other.value; }
};

ObjValue Parse(const formats::json::Value& value,
               formats::parse::To<ObjValue>) {
  ObjValue result{};
  result.value = value["obj"].As<SomeValue>();

  return result;
}

formats::json::Value Serialize(const ObjValue& value,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder;
  builder["obj"] = value.value;

  return builder.ExtractValue();
}

struct ArrayOfValues final {
  std::vector<ObjValue> values;
};

ArrayOfValues Parse(const formats::json::Value& value,
                    formats::parse::To<ArrayOfValues>) {
  ArrayOfValues result;
  result.values = value["data"].As<std::vector<ObjValue>>();

  return result;
}

formats::json::Value Serialize(const ArrayOfValues& value,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder;
  builder["data"] = value.values;

  return builder.ExtractValue();
}

struct Data final {
  ArrayOfValues values2;
  ArrayOfValues values1;

  bool operator==(const Data& other) const {
    return values1.values == other.values1.values &&
           values2.values == other.values2.values;
  }
};

Data Parse(const formats::json::Value& value, formats::parse::To<Data>) {
  Data result;
  result.values1 = value["values1"].As<ArrayOfValues>();
  result.values2 = value["values2"].As<ArrayOfValues>();

  return result;
}

formats::json::Value Serialize(const Data& value,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder;
  builder["values1"] = value.values1;
  builder["values2"] = value.values2;

  return builder.ExtractValue();
}

Data GenerateData(std::size_t entries) {
  Data result{};

  result.values1.values.reserve(entries);
  result.values2.values.reserve(entries);
  for (std::size_t i = 0; i < entries; ++i) {
    result.values1.values.push_back({i});
    result.values2.values.push_back({i});
  }

  return result;
}

void JsonParseValueDomLotsOfMissingKeys(benchmark::State& state) {
  const auto data = GenerateData(state.range(0));
  const auto json = formats::json::ValueBuilder{data}.ExtractValue();

  for ([[maybe_unused]] auto _ : state) {
    benchmark::DoNotOptimize(json.As<Data>());
  }
}
BENCHMARK(JsonParseValueDomLotsOfMissingKeys)
    ->RangeMultiplier(2)
    ->Range(1 << 7, 1 << 14);

}  // namespace

USERVER_NAMESPACE_END
