#include <userver/formats/yaml.hpp>

#include <chrono>
#include <limits>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <userver/utest/death_tests.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// clang-format off

/// [Sample yaml inline construction functions]
const auto kDoc = formats::yaml::MakeObject(
    "null", nullptr,
    "bool", true,
    "int", -1,
    "uint", 2u,
    "long", -3L,
    "ulong", 4ul,
    "long-long", -5LL,
    "ulong-long", 6ull,
    "double", 1.5,
    "float", 2.5f,
    "c-string", "value",
    "string", std::string{"string"},
    "string-view", std::string_view{"view"}
);
/// [Sample yaml inline construction functions]

// clang-format on
TEST(YamlInline, EmptyContainers) {
    EXPECT_TRUE(formats::yaml::MakeObject().IsObject());
    EXPECT_TRUE(formats::yaml::MakeObject().IsEmpty());
    EXPECT_TRUE(formats::yaml::MakeArray().IsArray());
    EXPECT_TRUE(formats::yaml::MakeArray().IsEmpty());
}

TEST(YamlInline, Object) {
    EXPECT_TRUE(kDoc["null"].IsNull());
    EXPECT_TRUE(kDoc["bool"].As<bool>());
    EXPECT_EQ(kDoc["int"].As<int>(), -1);
    EXPECT_EQ(kDoc["uint"].As<unsigned int>(), 2u);
    EXPECT_EQ(kDoc["long"].As<long>(), -3L);
    EXPECT_EQ(kDoc["ulong"].As<unsigned long>(), 4ul);
    EXPECT_EQ(kDoc["long-long"].As<long long>(), -5LL);
    EXPECT_EQ(kDoc["ulong-long"].As<unsigned long long>(), 6ull);
    EXPECT_DOUBLE_EQ(kDoc["double"].As<double>(), 1.5);
    EXPECT_DOUBLE_EQ(kDoc["float"].As<double>(), 2.5);
    EXPECT_EQ(kDoc["c-string"].As<std::string>(), "value");
    EXPECT_EQ(kDoc["string"].As<std::string>(), "string");
    EXPECT_EQ(kDoc["string-view"].As<std::string>(), "view");
}

TEST(YamlInline, ArrayAndNestedValues) {
    const auto nested = formats::yaml::MakeObject("key", "value");
    const auto value = formats::yaml::MakeArray(nullptr, false, 1, 2u, -3L, 4ul, -5LL, 6ull, 1.5, "text", nested);

    ASSERT_EQ(value.GetSize(), 11);
    EXPECT_TRUE(value[0].IsNull());
    EXPECT_FALSE(value[1].As<bool>());
    EXPECT_EQ(value[2].As<int>(), 1);
    EXPECT_EQ(value[3].As<unsigned int>(), 2u);
    EXPECT_EQ(value[4].As<long>(), -3L);
    EXPECT_EQ(value[5].As<unsigned long>(), 4ul);
    EXPECT_EQ(value[6].As<long long>(), -5LL);
    EXPECT_EQ(value[7].As<unsigned long long>(), 6ull);
    EXPECT_DOUBLE_EQ(value[8].As<double>(), 1.5);
    EXPECT_EQ(value[9].As<std::string>(), "text");
    EXPECT_EQ(value[10]["key"].As<std::string>(), "value");
}

TEST(YamlInline, TimePoint) {
    const auto epoch = std::chrono::system_clock::time_point{};
    const auto value = formats::yaml::MakeObject("time", epoch, "array", formats::yaml::MakeArray(epoch));

    EXPECT_EQ(value["time"].As<std::string>(), "1970-01-01T00:00:00+00:00");
    EXPECT_EQ(value["array"][0].As<std::string>(), "1970-01-01T00:00:00+00:00");
}

TEST(YamlInline, RejectsInvalidDouble) {
#ifdef NDEBUG
    EXPECT_THROW(formats::yaml::MakeObject("value", std::numeric_limits<double>::infinity()), formats::yaml::Exception);
    EXPECT_THROW(formats::yaml::MakeArray(std::numeric_limits<double>::quiet_NaN()), formats::yaml::Exception);
#else
    UEXPECT_DEATH(formats::yaml::MakeObject("value", std::numeric_limits<double>::infinity()), "inf");
    UEXPECT_DEATH(formats::yaml::MakeArray(std::numeric_limits<double>::quiet_NaN()), "nan");
#endif
}

}  // namespace

USERVER_NAMESPACE_END
