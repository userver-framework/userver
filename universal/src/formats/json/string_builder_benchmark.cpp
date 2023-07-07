#include <benchmark/benchmark.h>

#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

using namespace formats::json;

Value Build(int level) {
  ValueBuilder builder;
  if (level % 2) {
    builder.PushBack("abc");
    builder.PushBack(123);
    builder.PushBack(Value());
    if (level > 0) builder.PushBack(Build(level - 1));
  } else {
    builder["key"] = "abc";
    builder["other-key"] = 123;
    builder["super-key"] = Value();
    if (level > 0) builder["object"] = Build(level - 1);
  }

  return builder.ExtractValue();
}

void JsonSerialize(benchmark::State& state) {
  for (auto _ : state) {
    auto json = Build(state.range(0));
    const auto res = ToString(json);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(JsonSerialize)->RangeMultiplier(4)->Range(1, 1024);

void Write(int level, StringBuilder& sw) {
  if (level % 2) {
    StringBuilder::ArrayGuard guard(sw);
    sw.WriteString("abc");
    sw.WriteInt64(123);
    sw.WriteNull();
    if (level > 0) Write(level - 1, sw);
  } else {
    StringBuilder::ObjectGuard guard(sw);

    sw.Key("key");
    sw.WriteString("abc");

    sw.Key("other-key");
    sw.WriteInt64(123);

    sw.Key("super-key");
    sw.WriteNull();

    if (level > 0) {
      sw.Key("object");
      Write(level - 1, sw);
    }
  }
}

std::string WriteString(int level) {
  StringBuilder sw;
  Write(level, sw);
  return sw.GetString();
}

void JsonStringBuilder(benchmark::State& state) {
  for (auto _ : state) {
    auto str = WriteString(state.range(0));
    benchmark::DoNotOptimize(str);
  }
}
BENCHMARK(JsonStringBuilder)->RangeMultiplier(4)->Range(1, 1024);

USERVER_NAMESPACE_END
