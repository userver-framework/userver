#include <benchmark/benchmark.h>

#include <fmt/format.h>

#include <userver/formats/json/parser/parser.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/parse/common_containers.hpp>

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
  for (auto _ : state) {
    auto json = formats::json::FromString(input);
    const auto res = ParseDom(json);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(JsonParseArrayDom)->RangeMultiplier(4)->Range(1, 1024);

void JsonParseArraySax(benchmark::State& state) {
  const auto input = BuildArray(state.range(0));
  for (auto _ : state) {
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
  for (auto _ : state) {
    const auto res = formats::json::FromString(input);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(JsonParseValueDom)->RangeMultiplier(2)->Range(1, 16);

void JsonParseValueSax(benchmark::State& state) {
  const auto input = BuildObject(state.range(0));
  for (auto _ : state) {
    const auto res = formats::json::parser::ParseToType<
        formats::json::Value, formats::json::parser::JsonValueParser>(input);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(JsonParseValueSax)->RangeMultiplier(2)->Range(1, 16);

USERVER_NAMESPACE_END
