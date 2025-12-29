#include <gmock/gmock.h>
#include <userver/utest/assert_macros.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/chaotic/array.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

TEST(Array, OfInt) {
    const auto json = formats::json::MakeArray("foo", "bar");
    using Arr = chaotic::Array<std::string, std::vector<std::string>>;
    const std::vector<std::string> x = json.As<Arr>();

    ASSERT_THAT(x, ::testing::ElementsAre("foo", "bar"));
}

TEST(Array, OfIntSerializer) {
    const auto json = formats::json::MakeArray("foo", "bar");
    using Arr = chaotic::Array<std::string, std::vector<std::string>>;
    const std::vector<std::string> x = json.As<Arr>();

    auto result = formats::json::ValueBuilder{Arr{json.As<Arr>()}}.ExtractValue();
    EXPECT_EQ(result, json);
}

TEST(Array, OfIntWithValidators) {
    const auto json = formats::json::MakeArray("foo", "bar");
    using Arr = chaotic::Array<std::string, std::vector<std::string>, chaotic::MinItems<2>>;
    const std::vector<std::string> x = json.As<Arr>();
    ASSERT_THAT(x, ::testing::ElementsAre("foo", "bar"));

    const auto json1 = formats::json::MakeArray("foo");
    UEXPECT_THROW_MSG(
        json1.As<Arr>(),
        chaotic::Error<formats::json::Value>,
        "Error at path '/': Too short array, minimum length=2, given=1"
    );
}

TEST(Array, OfIntWithValidatorsSerializer) {
    const auto json = formats::json::MakeArray("foo", "bar");
    using Arr = chaotic::Array<std::string, std::vector<std::string>, chaotic::MinItems<2>>;
    EXPECT_EQ(formats::json::ValueBuilder{Arr{json.As<Arr>()}}.ExtractValue(), json);
}

TEST(Array, OfArrayOfString) {
    const auto json = formats::json::MakeArray(formats::json::MakeArray("foo"));

    using Arr =
        chaotic::Array<chaotic::Array<std::string, std::vector<std::string>>, std::vector<std::vector<std::string>>>;
    const std::vector<std::vector<std::string>> x = json.As<Arr>();
}

TEST(Array, OfArrayOfStringSerializer) {
    const auto json = formats::json::MakeArray(formats::json::MakeArray("foo"));

    using Arr =
        chaotic::Array<chaotic::Array<std::string, std::vector<std::string>>, std::vector<std::vector<std::string>>>;
    EXPECT_EQ(formats::json::ValueBuilder{Arr{json.As<Arr>()}}.ExtractValue(), json);
}

TEST(Array, OfEmptyArraySerializer) {
    const auto json = formats::json::MakeArray();
    using Arr = chaotic::Array<std::string, std::vector<std::string>>;
    EXPECT_EQ(formats::json::ValueBuilder{Arr{json.As<Arr>()}}.ExtractValue(), json);
}

TEST(Array, FromObjectShouldThrow) {
    const auto json = formats::json::MakeObject("lat", 56.008889, "lon", 92.871944);
    using Arr = chaotic::Array<double, std::vector<double>>;

    UEXPECT_THROW_MSG(
        json.As<Arr>(),
        formats::json::TypeMismatchException,
        "Error at path '/': Wrong type. Expected: kArrayValue, actual: kObjectValue"
    );
}

}  // namespace

USERVER_NAMESPACE_END
