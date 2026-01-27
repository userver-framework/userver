#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

template <typename T>
std::string GetFloatSpecialValue(const T val) {
    if (std::isnan(val)) {
        return "NaN";
    } else if (std::isinf(val)) {
        return val > 0 ? "Infinity" : "-Infinity";
    } else {
        throw std::runtime_error("not a special value");
    }
}

template <typename T>
class FloatSpecialToJsonTest : public ::testing::Test {
public:
    using Param = T;
};

using FpTypes = ::testing::Types<
    FloatQuietNan,
    FloatSignalingNan,
    FloatPositiveInfinity,
    FloatNegativeInfinity,
    DoubleQuietNan,
    DoubleSignalingNan,
    DoublePositiveInfinity,
    DoubleNegativeInfinity>;

TYPED_TEST_SUITE(FloatSpecialToJsonTest, FpTypes);

TYPED_TEST(FloatSpecialToJsonTest, Test) {
    using Param = typename TestFixture::Param;

    if constexpr (Param::kSkip) {
        return;
    }

    const auto& value = Param::kValue;
    auto input = PrepareTestData(value);
    formats::json::Value json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, {})));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input)));

    EXPECT_EQ(json["field1"].As<std::string>(), GetFloatSpecialValue(value.field1));
    EXPECT_EQ(json["field1"].As<std::string>(), sample_json["field1"].As<std::string>());
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
