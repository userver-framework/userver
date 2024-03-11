#include <string>

#include <gtest/gtest.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string MakeStringOfDeepObject(std::size_t depth) {
  std::string str;
  str.reserve(depth * 6 + 1);
  for (std::size_t i = 0; i < depth; ++i) {
    str += R"({"a":)";
  }
  str += "1";
  for (std::size_t i = 0; i < depth; ++i) {
    str += "}";
  }
  return str;
}

std::string MakeStringOfDeepArray(std::size_t depth) {
  std::string str;
  str.reserve(2 * depth + 1);
  for (std::size_t i = 0; i < depth; ++i) {
    str += "[";
  }
  str += "1";
  for (std::size_t i = 0; i < depth; ++i) {
    str += "]";
  }
  return str;
}

template <typename Func>
void RunStackLimitedTest(Func&& fn) {
  constexpr std::size_t kWorkerThreads = 1;
  engine::TaskProcessorPoolsConfig config;
  config.coro_stack_size = 128 * 1024ULL;

  constexpr std::size_t kDepth = 16 * 1024;

  engine::RunStandalone(kWorkerThreads, config,
                        [fn = std::forward<Func>(fn)] { fn(kDepth); });
}

formats::json::Value MakeDeepJson(std::size_t depth) {
  formats::json::ValueBuilder vb1;
  formats::json::ValueBuilder vb2;
  vb2["a"] = "b";

  while (depth--) {
    vb1["a"] = vb2.ExtractValue();
    vb2["a"] = vb1.ExtractValue();
  }

  return vb2.ExtractValue();
}

}  // namespace

TEST(FormatsJson, DeepObjectFromString) {
  RunStackLimitedTest([](std::size_t depth) {
    EXPECT_THROW(formats::json::FromString(MakeStringOfDeepObject(depth)),
                 formats::json::ParseException);
  });
}

TEST(FormatsJson, DeepArrayFromString) {
  RunStackLimitedTest([](std::size_t depth) {
    EXPECT_THROW(formats::json::FromString(MakeStringOfDeepArray(depth)),
                 formats::json::ParseException);
  });
}

TEST(FormatsJson, DeepObjectFromStringDuplicateKeys) {
  RunStackLimitedTest([](std::size_t depth) {
    auto json_str = MakeStringOfDeepObject(depth);
    // {a: {a: 1}} -> {a: {a: 1}
    json_str.pop_back();
    // {a: {a: 1} -> {a: {a: 1}, a: 1}
    json_str.append(R"(,"a":1})");

    EXPECT_THROW(formats::json::FromString(json_str),
                 formats::json::ParseException);
  });
}

TEST(FormatsJson, DeepObjectFromStringNonsense) {
  RunStackLimitedTest([](std::size_t depth) {
    auto json_str = MakeStringOfDeepObject(depth);
    // {a: {a: 1}} -> {a: {a: 1}
    json_str.pop_back();
    // {a: {a: 1} -> {a: {a: 1}, nonsense}
    json_str.append(R"(,"nonsense"})");

    EXPECT_THROW(formats::json::FromString(json_str),
                 formats::json::ParseException);
  });
}

TEST(FormatsJson, DeepObjectOperatorEqualsTo) {
  RunStackLimitedTest([](std::size_t depth) {
    auto v1 = MakeDeepJson(depth);
    auto v2 = MakeDeepJson(depth);

    EXPECT_EQ(v1, v2);
  });
}

USERVER_NAMESPACE_END
