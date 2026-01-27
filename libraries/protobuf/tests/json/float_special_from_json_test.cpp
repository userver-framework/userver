#include <gtest/gtest.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

template <typename T>
class FloatSpecialFromJsonTest : public ::testing::Test {
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

TYPED_TEST_SUITE(FloatSpecialFromJsonTest, FpTypes);

TYPED_TEST(FloatSpecialFromJsonTest, Test) {
    using Param = typename TestFixture::Param;
    using Message = typename Param::Message;

    if constexpr (Param::kSkip) {
        return;
    }

    const auto& json = Param::kJson;
    const auto& expected_data = Param::kValue;

    Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(json);
    expected_message = PrepareTestData(expected_data);

    message.set_field1(100.001);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input)));
    UASSERT_NO_THROW(InitSampleMessage(json, sample_message));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
