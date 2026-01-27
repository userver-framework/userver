#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>

#include <google/protobuf/util/time_util.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/proto-structs/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "messages.pb.h"
#include "structs.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(StructToMessage, Empty) {
    structs::Empty obj;
    messages::Empty msg;

    UASSERT_NO_THROW(StructToMessage(obj, msg));
    UASSERT_NO_THROW((msg = StructToMessage(obj)));
    UASSERT_NO_THROW((msg = StructToMessage(structs::Empty{obj})));
}

TEST(StructToMessage, Scalar) {
    {
        structs::Scalar obj{
            .f1 = true,
            .f2 = std::numeric_limits<int32_t>::min(),
            .f3 = std::numeric_limits<uint32_t>::max(),
            .f4 = std::numeric_limits<int64_t>::min(),
            .f5 = std::numeric_limits<uint64_t>::max(),
            .f6 = static_cast<float>(1.5),
            .f7 = -2.5,
            .f8 = "test1",
            .f9 = "test2",
            .f10 = structs::TestEnum::kValue1,
            .f11 = 987
        };

        auto msg = StructToMessage(obj);
        CheckScalarEqual(obj, msg);

        msg = StructToMessage(structs::Scalar{obj});
        CheckScalarEqual(obj, msg);
    }

    {
        structs::Scalar obj;
        messages::Scalar msg;

        obj.f2 = 1001;
        msg.set_f2(1);
        msg.set_f3(2);
        msg.set_f8("test");

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckScalarEqual(obj, msg);

        obj.f8 = "hello";
        obj.f9 = "world";
        obj.f11 = 654;

        UASSERT_NO_THROW(StructToMessage(structs::Scalar{obj}, msg));
        CheckScalarEqual(obj, msg);
    }

    if (std::numeric_limits<std::size_t>::max() > std::numeric_limits<std::int32_t>::max()) {
        structs::Scalar obj;
        obj.f11 = std::numeric_limits<std::size_t>::max();

        UEXPECT_THROW_MSG(static_cast<void>(StructToMessage(obj)), WriteError, "'messages.Scalar.f11'");
    }
}

TEST(StructToMessage, WellKnownStd) {
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
    auto create_valid = []() {
        return structs::WellKnownStd{.f3 = {std::chrono::year{1}, std::chrono::month{1}, std::chrono::day{1}}};
    };

    {
        structs::WellKnownStd obj{
            .f1 = TimePoint{std::chrono::milliseconds{123'456'789}},
            .f2 = std::chrono::milliseconds{987'654'321},
            .f3 = {std::chrono::year{2025}, std::chrono::month{8}, std::chrono::day{27}},
            .f4 =
                std::chrono::hh_mm_ss<std::chrono::microseconds>{
                    std::chrono::minutes(65) + std::chrono::seconds{13} + std::chrono::microseconds{123'456}
                }
        };

        auto msg = StructToMessage(obj);

        CheckWellKnownStdEqual(obj, msg);

        msg = StructToMessage(structs::WellKnownStd{obj});

        CheckWellKnownStdEqual(obj, msg);
    }

    {
        structs::WellKnownStd obj = create_valid();
        messages::WellKnownStd msg;

        obj.f1 = TimePoint{std::chrono::milliseconds{-987'654'321}};
        *msg.mutable_f1() = ::google::protobuf::util::TimeUtil::NanosecondsToTimestamp(123'456'789'987'654'321LL);
        *msg.mutable_f2() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(1001);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckWellKnownStdEqual(obj, msg);

        obj.f2 = std::chrono::milliseconds{-987'654'321};
        *msg.mutable_f2() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(123'456'789'987'654'321LL);

        UASSERT_NO_THROW(StructToMessage(structs::WellKnownStd{obj}, msg));
        CheckWellKnownStdEqual(obj, msg);
    }

    constexpr auto kMaxSecondsInStdTimePoint =
        (TimePoint::duration::max().count() / TimePoint::duration::period::den) * TimePoint::duration::period::num;

    if (kMaxSecondsInStdTimePoint > ::google::protobuf::util::TimeUtil::kTimestampMaxSeconds) {
        structs::WellKnownStd obj = create_valid();
        messages::WellKnownStd msg;

        obj.f1 = TimePoint{std::chrono::seconds{kMaxSecondsInStdTimePoint}};

        UEXPECT_THROW_MSG(static_cast<void>(StructToMessage(obj)), WriteError, "'messages.WellKnownStd.f1'");
    }

    if ((std::chrono::milliseconds::max().count() / 1000) > ::google::protobuf::util::TimeUtil::kDurationMaxSeconds) {
        structs::WellKnownStd obj = create_valid();
        messages::WellKnownStd msg;

        obj.f2 = std::chrono::milliseconds::max();

        UEXPECT_THROW_MSG(static_cast<void>(StructToMessage(obj)), WriteError, "'messages.WellKnownStd.f2'");
    }

    {
        structs::WellKnownStd obj = create_valid();
        obj.f3 = {std::chrono::year{2025}, std::chrono::month{2}, std::chrono::day{29}};

        UEXPECT_THROW_MSG(static_cast<void>(StructToMessage(obj)), WriteError, "'messages.WellKnownStd.f3'");
    }

    {
        structs::WellKnownStd obj = create_valid();
        obj.f4 = std::chrono::hh_mm_ss<std::chrono::microseconds>{std::chrono::hours{25}};

        UEXPECT_THROW_MSG(static_cast<void>(StructToMessage(obj)), WriteError, "'messages.WellKnownStd.f4'");
    }
}

TEST(StructToMessage, WellKnownUsrv) {
    using TimePoint = Timestamp::TimePoint;

    {
        structs::WellKnownUsrv obj{
            .f1 = structs::Simple{.f1 = 100},
            .f2 = TimePoint{std::chrono::milliseconds{123'456'789}},
            .f3 = std::chrono::milliseconds{987'654'321},
            .f4 = std::chrono::year_month_day{std::chrono::year{2025}, std::chrono::month{8}, std::chrono::day{27}},
            .f5 =
                std::chrono::hh_mm_ss<std::chrono::microseconds>{
                    std::chrono::minutes(65) + std::chrono::seconds{13} + std::chrono::microseconds{123'456}
                },
            .f6 =
                utils::datetime::TimeOfDay<std::chrono::microseconds>{
                    std::chrono::hours{23} + std::chrono::minutes{59} + std::chrono::seconds{59} +
                    std::chrono::microseconds{999'999}
                },
            .f7 = decimal64::Decimal<3>{"123.456"}
        };
        messages::Simple any_payload;

        auto msg = StructToMessage(obj);

        CheckWellKnownUsrvEqual(obj, msg);
        ASSERT_TRUE(msg.f1().UnpackTo(&any_payload));
        CheckSimpleEqual(obj.f1.Unpack<structs::Simple>(), any_payload);

        msg = StructToMessage(structs::WellKnownUsrv{obj});

        CheckWellKnownUsrvEqual(obj, msg);
        ASSERT_TRUE(msg.f1().UnpackTo(&any_payload));
        CheckSimpleEqual(obj.f1.Unpack<structs::Simple>(), any_payload);
    }

    {
        structs::WellKnownUsrv obj;
        messages::WellKnownUsrv msg;

        obj.f2 = TimePoint{std::chrono::milliseconds{-987'654'321}};
        obj.f7 = decimal64::Decimal<3>("1001.001");

        messages::Scalar any_payload;
        any_payload.set_f10(messages::TestEnum::TEST_ENUM_VALUE1);

        ASSERT_TRUE(msg.mutable_f1()->PackFrom(any_payload));
        *msg.mutable_f2() = ::google::protobuf::util::TimeUtil::NanosecondsToTimestamp(123'456'789'987'654'321LL);
        *msg.mutable_f3() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(1001);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckWellKnownUsrvEqual(obj, msg);

        UASSERT_NO_THROW((obj.f1 = structs::Scalar{.f2 = 5}));
        obj.f3 = std::chrono::milliseconds{-987'654'321};
        obj.f7 = decimal64::Decimal<3>("-1001.001");
        *msg.mutable_f3() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(123'456'789'987'654'321LL);

        UASSERT_NO_THROW(StructToMessage(structs::WellKnownUsrv{obj}, msg));
        CheckWellKnownUsrvEqual(obj, msg);
        ASSERT_TRUE(msg.f1().UnpackTo(&any_payload));
        CheckScalarEqual(obj.f1.Unpack<structs::Scalar>(), any_payload);
    }
}

TEST(StructToMessage, WellKnownJson) {
    const auto create_array = [](double num, const std::string& str, bool b) {
        formats::json::ValueBuilder builder{formats::common::Type::kArray};
        builder.PushBack(formats::json::ValueBuilder{formats::common::Type::kNull});
        builder.PushBack(num);
        builder.PushBack(str);
        builder.PushBack(b);
        return std::move(builder).ExtractValue();
    };

    const auto create_object = [](double num, const std::string& str, bool b) {
        formats::json::ValueBuilder builder{formats::common::Type::kObject};
        builder["key0"] = formats::json::ValueBuilder{formats::common::Type::kNull};
        builder["key1"] = num;
        builder["key2"] = str;
        builder["key3"] = b;
        return std::move(builder).ExtractValue();
    };

    {
        structs::WellKnownJson obj;
        auto msg = StructToMessage(obj);

        CheckWellKnownJsonEqual(obj, msg);
    }

    {
        structs::WellKnownJson obj = {.f1 = formats::json::ValueBuilder{1001}.ExtractValue()};
        auto msg = StructToMessage(obj);

        CheckWellKnownJsonEqual(obj, msg);
    }

    {
        structs::WellKnownJson obj = {.f2 = formats::json::Array{create_array(100, "test", false)}};
        auto msg = StructToMessage(obj);

        CheckWellKnownJsonEqual(obj, msg);
    }

    {
        structs::WellKnownJson obj = {.f3 = formats::json::Object{create_object(200, "oops", true)}};
        auto msg = StructToMessage(obj);

        CheckWellKnownJsonEqual(obj, msg);
    }

    {
        structs::WellKnownJson obj = {
            .f1 = formats::json::ValueBuilder{1001}.ExtractValue(),
            .f2 = formats::json::Array{create_array(10, "hello", true)},
            .f3 = formats::json::Object{create_object(20, "world", false)}
        };
        auto msg = StructToMessage(std::move(obj));

        CheckWellKnownJsonEqual(obj, msg);
    }

    {
        formats::json::ValueBuilder builder{formats::common::Type::kArray};
        builder.PushBack(create_array(5, "hola", true));
        structs::WellKnownJson obj = {.f1 = builder.ExtractValue()};
        auto msg = StructToMessage(std::move(obj));

        CheckWellKnownJsonEqual(obj, msg);
    }

    {
        formats::json::ValueBuilder builder{formats::common::Type::kObject};
        builder["top"] = create_object(6, "bonjour", false);
        structs::WellKnownJson obj = {.f1 = builder.ExtractValue()};
        auto msg = StructToMessage(std::move(obj));

        CheckWellKnownJsonEqual(obj, msg);
    }
}

TEST(StructToMessage, Optional) {
    {
        structs::Optional obj =
            {.f1 = 1001, .f2 = "test", .f3 = structs::TestEnum::kValue1, .f4 = structs::Simple{.f1 = 10}};

        auto msg = StructToMessage(obj);
        CheckOptionalEqual(obj, msg);

        msg = StructToMessage(structs::Optional{obj});
        CheckOptionalEqual(obj, msg);
    }

    {
        structs::Optional obj;
        messages::Optional msg;

        obj.f1 = 1001;
        obj.f4 = {.f1 = 13};
        msg.set_f2("test2");
        msg.set_f3(messages::TestEnum::TEST_ENUM_VALUE2);
        msg.mutable_f4()->set_f1(5);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckOptionalEqual(obj, msg);

        obj.f2 = "test3";

        UASSERT_NO_THROW(StructToMessage(structs::Optional{obj}, msg));
        CheckOptionalEqual(obj, msg);
    }
}

TEST(StructToMessage, Repeated) {
    {
        structs::Repeated obj;
        obj.f1 = {10, 11, 12};
        obj.f2 = {"test1", "test2"};
        obj.f3 = {
            structs::TestEnum::kValue1,
            structs::TestEnum::kUnspecified,
            structs::TestEnum::kValue2,
            static_cast<structs::TestEnum>(1001)
        };
        obj.f4 = {{.f1 = 1000}, {.f1 = 1001}};

        auto msg = StructToMessage(obj);
        CheckRepeatedEqual(obj, msg);

        msg = StructToMessage(structs::Repeated{obj});
        CheckRepeatedEqual(obj, msg);
    }

    {
        structs::Repeated obj;
        messages::Repeated msg;

        obj.f1 = {1};
        obj.f4 = {{.f1 = 13}};
        msg.add_f1(1);
        msg.add_f1(2);
        msg.add_f1(3);
        msg.add_f2("test2");
        msg.mutable_f4()->Add()->set_f1(5);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckRepeatedEqual(obj, msg);

        obj.f2.push_back("hello");

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckRepeatedEqual(obj, msg);
    }
}

TEST(StructToMessage, Map) {
    {
        structs::Map obj;
        obj.f1 = {{5, 15}, {6, 16}, {7, 17}};
        obj.f2 = {{"key1", "value1"}, {"key2", "value2"}};
        obj.f3 = {{true, structs::TestEnum::kUnspecified}, {false, structs::TestEnum::kValue2}};
        obj.f4 = {{"simple1", {.f1 = 1001}}, {"simple2", {.f1 = 1002}}};

        auto msg = StructToMessage(obj);
        CheckMapEqual(obj, msg);

        msg = StructToMessage(structs::Map{obj});
        CheckMapEqual(obj, msg);
    }

    {
        structs::Map obj;
        messages::Map msg;

        obj.f1 = {{5, 15}};
        obj.f4 = {{"simple1", {.f1 = 1001}}};
        (*msg.mutable_f1())[1] = 1;
        (*msg.mutable_f1())[2] = 2;
        (*msg.mutable_f1())[3] = 3;
        (*msg.mutable_f2())["key1"] = "value1";
        (*msg.mutable_f4())["simple1"].set_f1(1001);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckMapEqual(obj, msg);

        obj.f2.emplace("key2", "value2");

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckMapEqual(obj, msg);
    }
}

TEST(StructToMessage, Oneof) {
    {
        structs::Oneof obj;
        obj.test_oneof.Set<0>(1001);

        auto msg = StructToMessage(obj);
        CheckOneofEqual(obj, msg);

        msg = StructToMessage(structs::Oneof{obj});
        CheckOneofEqual(obj, msg);
    }

    {
        structs::Oneof obj;
        messages::Oneof msg;

        obj.test_oneof.Set<1>("test");
        msg.set_f1(1);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckOneofEqual(obj, msg);

        obj.test_oneof.Set<2>(structs::TestEnum::kUnspecified);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckOneofEqual(obj, msg);

        obj.test_oneof.Set<3>({.f1 = 1001});

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckOneofEqual(obj, msg);
    }
}

TEST(StructToMessage, Indirect) {
    {
        structs::Indirect obj;
        obj.f1 = {.f1 = 1};
        obj.f2 = std::chrono::nanoseconds{987'654'321'123'456'789LL};
        obj.f3 = {structs::Simple{.f1 = 3}, structs::Simple{.f1 = 4}};
        obj.f4 = {{1, structs::Simple{.f1 = 5}}, {2, structs::Simple{.f1 = 6}}};
        obj.test_oneof.Set<0>(structs::Simple{.f1 = 7});
        obj.f7 = 8;
        obj.f8 = {structs::TestEnum::kValue1, structs::TestEnum::kValue2};
        obj.f9 = {{"hello", structs::Simple{.f1 = 9}}, {"", structs::Simple{.f1 = 10}}};

        auto msg = StructToMessage(obj);
        CheckIndirectEqual(obj, msg);

        msg = StructToMessage(structs::Indirect{obj});
        CheckIndirectEqual(obj, msg);
    }

    {
        structs::Indirect obj;
        messages::Indirect msg;

        obj.f1 = {.f1 = 1};
        obj.f4 = {{1, structs::Simple{.f1 = 10}}};
        obj.test_oneof.Set<1>("test");
        msg.mutable_f1()->set_f1(1001);
        msg.mutable_f3()->Add()->set_f1(1002);
        msg.mutable_f5()->set_f1(1003);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckIndirectEqual(obj, msg);

        obj.test_oneof.Set<0>(structs::Simple{.f1 = 11});
        msg.add_f8(messages::TEST_ENUM_VALUE1);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckIndirectEqual(obj, msg);
    }
}

TEST(StructToMessage, Strong) {
    {
        structs::Strong obj;
        obj.f1 = structs::Strong::F1Strong{1};
        obj.f2 = structs::Strong::F2Strong{"hello"};
        obj.f3 = {
            structs::Strong::F3Strong{structs::TestEnum::kValue1},
            structs::Strong::F3Strong{structs::TestEnum::kValue2}
        };
        obj.f4 = {
            {1, structs::Strong::F4Strong(structs::Simple{.f1 = 3})},
            {2, structs::Strong::F4Strong(structs::Simple{.f1 = 4})}
        };
        obj.test_oneof.Set<0>(structs::Strong::F5Strong{std::chrono::nanoseconds{-123'456'789'987'654'321}});

        auto msg = StructToMessage(obj);
        CheckStrongEqual(obj, msg);

        msg = StructToMessage(structs::Strong{obj});
        CheckStrongEqual(obj, msg);
    }

    {
        structs::Strong obj;
        messages::Strong msg;

        obj.f1 = structs::Strong::F1Strong{1};
        obj.f4 = {
            {100, structs::Strong::F4Strong(structs::Simple{.f1 = 2})},
            {200, structs::Strong::F4Strong(structs::Simple{.f1 = 3})}
        };
        msg.set_f1(1001);
        msg.add_f3(messages::TestEnum::TEST_ENUM_VALUE2);

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckStrongEqual(obj, msg);

        obj.test_oneof.Set<0>(structs::Strong::F5Strong{std::chrono::nanoseconds{123'456'789'987'654'321}});

        UASSERT_NO_THROW(StructToMessage(obj, msg));
        CheckStrongEqual(obj, msg);
    }
}

TEST(StructToMessage, Erroneous) {
    {
        structs::Erroneous obj;
        messages::Erroneous msg;

        EXPECT_NO_THROW(msg = StructToMessage(obj));
    }

    {
        structs::Erroneous obj;
        obj.f1 = {.error_type = structs::ConversionFailureType::kException};

        try {
            auto result = StructToMessage(obj);
            FAIL() << "exception should be thrown";
        } catch (const WriteError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Erroneous.f1'"));
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conversion_failure_exception"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }

    {
        structs::Erroneous obj;
        obj.f2 = {{.error_type = structs::ConversionFailureType::kError}};

        try {
            auto result = StructToMessage(std::move(obj));
            FAIL() << "exception should be thrown";
        } catch (const WriteError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Erroneous.f2.error_field'"));
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conversion_failure_error"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }

    {
        structs::Erroneous obj;
        obj.f3 = {{10, {.error_type = structs::ConversionFailureType::kErrorWithUnknownField}}};

        try {
            auto result = StructToMessage(obj);
            FAIL() << "exception should be thrown";
        } catch (const WriteError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Erroneous.f3.<unknown_1001>'"));
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conversion_failure_error_with_unknown_field"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }

    {
        structs::Erroneous obj;
        obj.test_oneof.Set<0>({.error_type = structs::ConversionFailureType::kErrorWithUnknownField});

        try {
            auto result = StructToMessage(obj);
            FAIL() << "exception should be thrown";
        } catch (const WriteError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Erroneous.f4.<unknown_1001>'"));
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conversion_failure_error_with_unknown_field"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
