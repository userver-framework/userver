#include <userver/formats/bson/serialize.hpp>

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

#include <fmt/format.h>

#include <gmock/gmock.h>

#include <userver/formats/bson.hpp>
#include <userver/formats/json.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace fb = formats::bson;
namespace fj = formats::json;

const auto kTimePoint =
    std::chrono::system_clock::from_time_t(0) + std::chrono::milliseconds{1};

const auto kSimpleJson =
    fj::MakeObject("string", "test", "int", int64_t{1}, "double", 1.1,
                   "int64max", std::numeric_limits<std::int16_t>::max(),
                   "int64min", std::numeric_limits<std::int16_t>::min());

const auto kSimpleDoc =
    fb::MakeDoc("double", 1.1, "int", int64_t{1}, "string", "test", "int64max",
                std::numeric_limits<std::int16_t>::max(), "int64min",
                std::numeric_limits<std::int16_t>::min());

const auto kSimpleJsonArray = fj::MakeArray("test", int64_t{1}, 1.0);

const auto kSimpleBsonArray = fb::MakeArray("test", int64_t{1}, 1.0);

std::optional<std::string> ComparePrimitivesWithReason(
    const fb::Value& arg, const fb::Value& expected) {
  if (expected.IsArray()) {
    return "Primitive expected value is array";
  }
  if (expected.IsDocument()) {
    return "Primitive expected value is document";
  }
  if (arg.IsArray()) {
    return "Primitive argument value is array";
  }
  if (arg.IsDocument()) {
    return "Primitive argument value is document";
  }
  if (expected.IsBool()) {
    if (!arg.IsBool()) {
      return "Primitive mismatch types: expected bool";
    }
    if (expected.As<bool>() != arg.As<bool>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsInt32()) {
    if (expected.As<int32_t>() != arg.As<int32_t>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsInt64()) {
    if (expected.As<int64_t>() != arg.As<int64_t>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsDouble()) {
    if (std::abs(expected.As<double>() - arg.As<double>()) > 1e-6) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsTimestamp()) {
    if (!arg.IsTimestamp()) {
      return "Primitive mismatch types: expected ts";
    }
    if (expected.As<fb::Timestamp>() != arg.As<fb::Timestamp>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsString()) {
    if (!arg.IsString()) {
      return "Primitive mismatch types: expected ts";
    }
    if (expected.As<std::string>() != arg.As<std::string>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsDecimal128()) {
    if (!arg.IsDecimal128()) {
      return "Primitive mismatch types: expected decimal";
    }
    if (expected.As<fb::Decimal128>() != arg.As<fb::Decimal128>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsDateTime()) {
    if (!arg.IsDateTime()) {
      return "Primitive mismatch types: expected datetime";
    }
    if (expected.As<std::chrono::system_clock::time_point>() !=
        arg.As<std::chrono::system_clock::time_point>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsOid()) {
    if (!arg.IsOid()) {
      return "Primitive mismatch types: expected oid";
    }
    if (expected.As<fb::Oid>() != arg.As<fb::Oid>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsBinary()) {
    if (!arg.IsBinary()) {
      return "Primitive mismatch types: expected binary";
    }
    if (expected.As<fb::Binary>() != arg.As<fb::Binary>()) {
      return "Primitive argument value mismatch";
    }
  }
  if (expected.IsMinKey()) {
    if (!arg.IsMinKey()) {
      return "Primitive mismatch types: expected minKey";
    }
  }
  if (expected.IsMaxKey()) {
    if (!arg.IsMaxKey()) {
      return "Primitive mismatch types: expected maxKey";
    }
  }
  return std::nullopt;
}

std::optional<std::string> CompareDocsWithReason(const fb::Value& arg,
                                                 const fb::Value& expected) {
  if (!expected.IsDocument() && !expected.IsArray()) {
    if (auto reason = ComparePrimitivesWithReason(arg, expected)) {
      return reason;
    }
    return std::nullopt;
  }
  if (expected.IsDocument()) {
    if (!arg.IsDocument()) {
      return "Mismatch types: expected document";
    }
    for (auto it = expected.begin(); it != expected.end(); ++it) {
      if (!arg.HasMember(it.GetName())) {
        return fmt::format("Argument doesn't have member {}", it.GetName());
      }
      if (auto reason = CompareDocsWithReason(*it, arg[it.GetName()])) {
        return reason;
      }
    }
    return std::nullopt;
  }
  if (!arg.IsArray()) {
    return "Mismatch types: expected array";
  }
  if (expected.GetSize() != arg.GetSize()) {
    return "Array size mismatch";
  }
  for (auto it = expected.begin(); it != expected.end(); ++it) {
    if (auto reason = CompareDocsWithReason(*it, arg[it.GetIndex()])) {
      return reason;
    }
  }
  return std::nullopt;
}

MATCHER_P(BsonMatcher, expected, "Bson matcher") {
  if (auto reason = CompareDocsWithReason(arg, expected)) {
    *result_listener << *reason;
    return false;
  }
  return true;
}

MATCHER(BsonPrimitiveMatcherFromJson,
        "Matcher for converting primitive from json to bson") {
  auto b = fb::ValueBuilder(arg).ExtractValue();
  auto j = fj::ValueBuilder(arg).ExtractValue();
  if (auto reason = CompareDocsWithReason(b, j.ConvertTo<fb::Value>())) {
    *result_listener << *reason;
    return false;
  }
  return true;
}

MATCHER(JsonPrimitiveMatcherFromBson,
        "Matcher for converting primitive from bson to json") {
  auto b = fb::ValueBuilder(arg).ExtractValue();
  auto j = fj::ValueBuilder(arg).ExtractValue();
  return b.ConvertTo<fj::Value>() == j;
}

MATCHER(JsonPrimitiveToStringMatcherFromBson,
        "Matcher for converting mongo primitive (oid, binary) from bson to "
        "json using ToString") {
  auto b = fb::ValueBuilder(arg).ExtractValue();
  auto j = fj::ValueBuilder(arg.ToString()).ExtractValue();
  return b.ConvertTo<fj::Value>() == j;
}

}  // namespace

TEST(Convert, PrimitiveToJson) {
  auto tp = std::chrono::system_clock::time_point{
      std::chrono::milliseconds{1554138241}};

  EXPECT_THAT(formats::common::Type::kNull, JsonPrimitiveMatcherFromBson());
  EXPECT_THAT(false, JsonPrimitiveMatcherFromBson());
  EXPECT_THAT(int32_t{2}, JsonPrimitiveMatcherFromBson());
  EXPECT_THAT(int64_t{2}, JsonPrimitiveMatcherFromBson());
  EXPECT_THAT(int64_t{3'000'000}, JsonPrimitiveMatcherFromBson());
  EXPECT_THAT(1.12, JsonPrimitiveMatcherFromBson());
  EXPECT_THAT(1., JsonPrimitiveMatcherFromBson());
  EXPECT_THAT(std::string{"test"}, JsonPrimitiveMatcherFromBson());
  EXPECT_THAT(tp, JsonPrimitiveMatcherFromBson());

  EXPECT_THAT(
      fb::Decimal128("1" + std::to_string(std::numeric_limits<int64_t>::max())),
      JsonPrimitiveToStringMatcherFromBsonMatcher());
  EXPECT_THAT(fb::Oid::MakeMinimalFor(tp),
              JsonPrimitiveToStringMatcherFromBsonMatcher());
  EXPECT_THAT(fb::Binary("some_binary_data"),
              JsonPrimitiveToStringMatcherFromBsonMatcher());

  {
    fb::Timestamp value(1554138241, 1);
    auto b = fb::ValueBuilder(value).ExtractValue();
    auto j = fj::ValueBuilder(value.GetTimestamp()).ExtractValue();
    EXPECT_EQ(b.ConvertTo<fj::Value>(), j);
  }
  {
    fb::MinKey value;
    auto b = fb::ValueBuilder(value).ExtractValue();
    auto j = fj::MakeObject("$minKey", 1);
    EXPECT_EQ(b.ConvertTo<fj::Value>(), j);
  }
  {
    fb::MaxKey value;
    auto b = fb::ValueBuilder(value).ExtractValue();
    auto j = fj::MakeObject("$maxKey", 1);
    EXPECT_EQ(b.ConvertTo<fj::Value>(), j);
  }
}

TEST(Convert, PrimitiveFromJson) {
  EXPECT_THAT(formats::common::Type::kNull, BsonPrimitiveMatcherFromJson());
  EXPECT_THAT(false, BsonPrimitiveMatcherFromJson());
  EXPECT_THAT(int32_t{2}, BsonPrimitiveMatcherFromJson());
  EXPECT_THAT(int64_t{2}, BsonPrimitiveMatcherFromJson());
  EXPECT_THAT(int64_t{3'000'000}, BsonPrimitiveMatcherFromJson());
  EXPECT_THAT(uint64_t{3'000'000}, BsonPrimitiveMatcherFromJson());
  EXPECT_THAT(1.12, BsonPrimitiveMatcherFromJson());
  EXPECT_THAT(1., BsonPrimitiveMatcherFromJson());
  EXPECT_THAT(std::string{"test"}, BsonPrimitiveMatcherFromJson());

  {
    auto value = std::chrono::system_clock::time_point{
        std::chrono::milliseconds{1554138241}};
    auto j = fj::ValueBuilder(value).ExtractValue();
    auto b = fb::ValueBuilder(j.As<std::string>()).ExtractValue();
    EXPECT_THAT(b, BsonMatcher(j.ConvertTo<fb::Value>()));
  }

  EXPECT_THROW(fj::ValueBuilder(std::numeric_limits<uint64_t>::max())
                   .ExtractValue()
                   .ConvertTo<fb::Value>(),
               fb::BsonException);
}

TEST(Convert, SimpleToJson) {
  auto json = kSimpleDoc.ConvertTo<fj::Value>();
  EXPECT_EQ(json, kSimpleJson);
}

TEST(Convert, SimpleFromJson) {
  auto bson = kSimpleJson.ConvertTo<fb::Value>();
  EXPECT_THAT(bson, BsonMatcher(kSimpleDoc));
}

TEST(Convert, SimpleToJsonArray) {
  auto json = kSimpleBsonArray.ConvertTo<fj::Value>();
  EXPECT_EQ(json, kSimpleJsonArray);
}

TEST(Convert, SimpleFromJsonArray) {
  auto bson = kSimpleJsonArray.ConvertTo<fb::Value>();
  EXPECT_THAT(bson, BsonMatcher(kSimpleBsonArray));
}

TEST(Convert, ToJson) {
  fb::ValueBuilder bb = kSimpleDoc;
  bb["simple_doc"] = kSimpleDoc;
  bb["empty_doc"] = fb::MakeDoc();
  bb["simple_array"] = kSimpleBsonArray;
  bb["empty_array"] = fb::MakeArray();
  auto json = bb.ExtractValue().ConvertTo<fj::Value>();
  fj::ValueBuilder jb = kSimpleJson;
  jb["simple_doc"] = kSimpleJson;
  jb["empty_doc"] = fj::MakeObject();
  jb["simple_array"] = kSimpleJsonArray;
  jb["empty_array"] = fj::MakeArray();
  EXPECT_EQ(json, jb.ExtractValue());
}

TEST(Convert, FromJson) {
  fj::ValueBuilder jb = kSimpleJson;
  jb["simple_doc"] = kSimpleJson;
  jb["empty_doc"] = fj::MakeObject();
  jb["simple_array"] = kSimpleJsonArray;
  jb["empty_array"] = fj::MakeArray();
  auto bson = jb.ExtractValue().ConvertTo<fb::Value>();
  fb::ValueBuilder bb = kSimpleDoc;
  bb["simple_doc"] = kSimpleDoc;
  bb["empty_doc"] = fb::MakeDoc();
  bb["simple_array"] = kSimpleBsonArray;
  bb["empty_array"] = fb::MakeArray();
  EXPECT_THAT(bson, BsonMatcher(bb.ExtractValue()));
}

USERVER_NAMESPACE_END
