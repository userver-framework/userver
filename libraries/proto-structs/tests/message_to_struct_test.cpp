#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>

#include <google/protobuf/util/time_util.h>

#include <userver/proto-structs/convert.hpp>

#include "messages.pb.h"
#include "structs.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(MessageToStruct, Empty) {
    messages::Empty msg;
    structs::Empty obj;

    ASSERT_NO_THROW(MessageToStruct(msg, obj));
    ASSERT_NO_THROW((obj = MessageToStruct<structs::Empty>(msg)));
}

TEST(MessageToStruct, Scalar) {
    {
        messages::Scalar msg;
        msg.set_f1(true);
        msg.set_f2(std::numeric_limits<int32_t>::min());
        msg.set_f3(std::numeric_limits<uint32_t>::max());
        msg.set_f4(std::numeric_limits<int64_t>::min());
        msg.set_f5(std::numeric_limits<uint64_t>::max());
        msg.set_f6(static_cast<float>(1.5));
        msg.set_f7(-2.5);
        msg.set_f8("test1");
        msg.set_f9("test2");
        msg.set_f10(messages::TestEnum::TEST_ENUM_VALUE1);
        msg.set_f11(123);

        auto obj = MessageToStruct<structs::Scalar>(msg);

        CheckScalarEqual(obj, msg);
    }

    {
        messages::Scalar msg;
        structs::Scalar obj;

        CheckScalarEqual(obj, msg);

        msg.set_f2(1001);
        obj.f2 = 1;
        obj.f3 = 2;
        obj.f8 = "test";

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckScalarEqual(obj, msg);

        msg.set_f8("hello");
        msg.set_f9("world");
        msg.set_f11(456);

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckScalarEqual(obj, msg);
    }

    {
        messages::Scalar msg;
        msg.set_f11(-1);

        try {
            auto result = MessageToStruct<structs::Scalar>(msg);
            FAIL() << "exception should be thrown";
        } catch (const ups::ReadError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Scalar.f11'"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }
}

TEST(MessageToStruct, WellKnownStd) {
    auto CreateValid = []() {
        messages::WellKnownStd msg;
        msg.mutable_f3()->set_year(1);
        msg.mutable_f3()->set_month(1);
        msg.mutable_f3()->set_day(1);
        return msg;
    };

    ASSERT_NO_THROW(static_cast<void>(MessageToStruct<structs::WellKnownStd>(CreateValid())));

    {
        messages::WellKnownStd msg;
        *msg.mutable_f1() = ::google::protobuf::util::TimeUtil::NanosecondsToTimestamp(123'456'789'987'654'321LL);
        *msg.mutable_f2() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(987'654'321'123'456'789LL);
        msg.mutable_f3()->set_year(2025);
        msg.mutable_f3()->set_month(8);
        msg.mutable_f3()->set_day(27);
        msg.mutable_f4()->set_hours(19);
        msg.mutable_f4()->set_minutes(30);
        msg.mutable_f4()->set_seconds(50);
        msg.mutable_f4()->set_nanos(123'456'789);

        auto obj = MessageToStruct<structs::WellKnownStd>(msg);

        CheckWellKnownStdEqual(obj, msg);
    }

    {
        messages::WellKnownStd msg = CreateValid();
        structs::WellKnownStd obj;

        *msg.mutable_f1() = ::google::protobuf::util::TimeUtil::NanosecondsToTimestamp(-987'654'321'123'456'789LL);
        obj.f1 = std::chrono::system_clock::now();
        obj.f2 = std::chrono::milliseconds(1001);

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckWellKnownStdEqual(obj, msg);

        *msg.mutable_f2() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(-123'456'789'987'654'321LL);
        msg.mutable_f4()->set_hours(24);
        msg.mutable_f4()->set_minutes(0);
        msg.mutable_f4()->set_seconds(0);
        msg.mutable_f4()->set_nanos(0);

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckWellKnownStdEqual(obj, msg);
    }

    {
        messages::WellKnownStd msg = CreateValid();
        msg.mutable_f1()->set_nanos(1'999'999'999);

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownStd>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownStd.f1'"))
        );
    }

    {
        messages::WellKnownStd msg = CreateValid();
        msg.mutable_f2()->set_nanos(-1'999'999'999);

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownStd>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownStd.f2'"))
        );
    }

    {
        messages::WellKnownStd msg = CreateValid();
        msg.mutable_f3()->set_day(0);  // not full date is not allowed for std::chrono::year_month_day

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownStd>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownStd.f3'"))
        );
    }

    {
        messages::WellKnownStd msg = CreateValid();
        msg.mutable_f4()->set_seconds(60);  // leap second is not allowed for std::chrono::hh_mm_ss

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownStd>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownStd.f4'"))
        );
    }
}

TEST(MessageToStruct, WellKnownUsrv) {
    auto CreateValid = []() {
        messages::WellKnownUsrv msg;
        msg.mutable_f7()->set_value("0.000");
        return msg;
    };

    ASSERT_NO_THROW(static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(CreateValid())));

    {
        messages::WellKnownUsrv msg;
        messages::Simple any_payload;

        any_payload.set_f1(100);

        ASSERT_TRUE(msg.mutable_f1()->PackFrom(any_payload));
        *msg.mutable_f2() = ::google::protobuf::util::TimeUtil::NanosecondsToTimestamp(123'456'789'987'654'321LL);
        *msg.mutable_f3() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(987'654'321'123'456'789LL);
        msg.mutable_f4()->set_year(2025);
        msg.mutable_f4()->set_month(8);
        msg.mutable_f4()->set_day(27);
        msg.mutable_f5()->set_hours(19);
        msg.mutable_f5()->set_minutes(30);
        msg.mutable_f5()->set_seconds(50);
        msg.mutable_f5()->set_nanos(123'456'789);
        msg.mutable_f6()->set_hours(2);
        msg.mutable_f6()->set_minutes(10);
        msg.mutable_f6()->set_seconds(11);
        msg.mutable_f6()->set_nanos(987'654'321);
        msg.mutable_f7()->set_value("1.123");

        auto obj = MessageToStruct<structs::WellKnownUsrv>(msg);

        CheckWellKnownUsrvEqual(obj, msg);
        CheckSimpleEqual(obj.f1.Unpack<structs::Simple>(), any_payload);
    }

    {
        messages::WellKnownUsrv msg = CreateValid();
        structs::WellKnownUsrv obj;
        messages::Simple any_payload;

        *msg.mutable_f2() = ::google::protobuf::util::TimeUtil::NanosecondsToTimestamp(-987'654'321'123'456'789LL);
        msg.mutable_f7()->set_value("1001.001");
        obj.f1.Pack<structs::Scalar>({.f10 = structs::TestEnum::kValue1});
        obj.f2 = std::chrono::system_clock::now();
        obj.f3 = std::chrono::milliseconds(1001);

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckWellKnownUsrvEqual(obj, msg);

        any_payload.set_f1(5);
        ASSERT_TRUE(msg.mutable_f1()->PackFrom(any_payload));

        *msg.mutable_f3() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(-123'456'789'987'654'321LL);
        msg.mutable_f5()->set_hours(24);
        msg.mutable_f5()->set_minutes(0);
        msg.mutable_f5()->set_seconds(0);
        msg.mutable_f5()->set_nanos(0);
        msg.mutable_f7()->set_value("-1001.001");

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckWellKnownUsrvEqual(obj, msg);
        CheckSimpleEqual(obj.f1.Unpack<structs::Simple>(), any_payload);
    }

    {
        messages::WellKnownUsrv msg = CreateValid();
        msg.mutable_f2()->set_nanos(1'999'999'999);

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownUsrv.f2'"))
        );
    }

    {
        messages::WellKnownUsrv msg = CreateValid();
        msg.mutable_f3()->set_nanos(-1'999'999'999);

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownUsrv.f3'"))
        );
    }

    {
        messages::WellKnownUsrv msg = CreateValid();
        msg.mutable_f4()->set_year(10'000);

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownUsrv.f4'"))
        );
    }

    {
        messages::WellKnownUsrv msg = CreateValid();
        msg.mutable_f5()->set_hours(-1);

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownUsrv.f5'"))
        );
    }

    {
        messages::WellKnownUsrv msg = CreateValid();
        msg.mutable_f6()->set_hours(24);

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownUsrv.f6'"))
        );

        msg = CreateValid();
        msg.mutable_f6()->set_seconds(60);

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownUsrv.f6'"))
        );
    }

    {
        messages::WellKnownUsrv msg = CreateValid();
        msg.mutable_f7()->set_value("");

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownUsrv.f7'"))
        );

        msg.Clear();
        msg.mutable_f7()->set_value("1.1234");

        EXPECT_THAT(
            [&msg]() { static_cast<void>(MessageToStruct<structs::WellKnownUsrv>(msg)); },
            ::testing::ThrowsMessage<ReadError>(::testing::HasSubstr("'messages.WellKnownUsrv.f7'"))
        );
    }
}

TEST(MessageToStruct, Optional) {
    {
        messages::Optional msg;
        msg.set_f1(1001);
        msg.set_f2("test");
        msg.set_f3(messages::TestEnum::TEST_ENUM_VALUE1);
        msg.mutable_f4()->set_f1(10);

        auto obj = MessageToStruct<structs::Optional>(msg);

        CheckOptionalEqual(obj, msg);
    }

    {
        messages::Optional msg;
        structs::Optional obj;

        CheckOptionalEqual(obj, msg);

        msg.set_f1(1001);
        msg.mutable_f4()->set_f1(13);
        obj.f2 = "test2";
        obj.f3 = structs::TestEnum::kValue2;
        obj.f4 = {.f1 = 5};

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckOptionalEqual(obj, msg);

        msg.set_f2("test3");

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckOptionalEqual(obj, msg);
    }
}

TEST(MessageToStruct, Repeated) {
    {
        messages::Repeated msg;
        msg.add_f1(10);
        msg.add_f1(11);
        msg.add_f1(12);
        msg.add_f2("test1");
        msg.add_f2("test2");
        msg.add_f3(messages::TestEnum::TEST_ENUM_VALUE1);
        msg.add_f3(messages::TestEnum::TEST_ENUM_UNSPECIFIED);
        msg.add_f3(messages::TestEnum::TEST_ENUM_VALUE2);
        msg.add_f3(static_cast<messages::TestEnum>(1001));
        msg.mutable_f4()->Add()->set_f1(1000);
        msg.mutable_f4()->Add()->set_f1(1001);

        auto obj = MessageToStruct<structs::Repeated>(msg);

        CheckRepeatedEqual(obj, msg);
    }

    {
        messages::Repeated msg;
        structs::Repeated obj;

        CheckRepeatedEqual(obj, msg);

        msg.add_f1(1);
        msg.mutable_f4()->Add()->set_f1(13);
        obj.f1 = {1, 2, 3};
        obj.f2 = {"test2"};
        obj.f4 = {{.f1 = 5}};

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckRepeatedEqual(obj, msg);

        msg.add_f2("hello");

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckRepeatedEqual(obj, msg);
    }
}

TEST(MessageToStruct, Map) {
    {
        messages::Map msg;
        (*msg.mutable_f1())[5] = 15;
        (*msg.mutable_f1())[6] = 16;
        (*msg.mutable_f1())[7] = 17;
        (*msg.mutable_f2())["key1"] = "value1";
        (*msg.mutable_f2())["key2"] = "value2";
        (*msg.mutable_f3())[false] = messages::TestEnum::TEST_ENUM_UNSPECIFIED;
        (*msg.mutable_f3())[true] = messages::TestEnum::TEST_ENUM_VALUE2;
        (*msg.mutable_f4())["simple1"].set_f1(1001);
        (*msg.mutable_f4())["simple2"].set_f1(1002);

        auto obj = MessageToStruct<structs::Map>(msg);

        CheckMapEqual(obj, msg);
    }

    {
        messages::Map msg;
        structs::Map obj;

        CheckMapEqual(obj, msg);

        (*msg.mutable_f1())[5] = 15;
        (*msg.mutable_f4())["simple1"].set_f1(1001);
        obj.f1 = {{1, 1}, {2, 2}, {3, 3}};
        obj.f2 = {{"key1", "value1"}};
        obj.f4 = {{"simple1", {.f1 = 5}}};

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckMapEqual(obj, msg);

        (*msg.mutable_f2())["key2"] = "value2";

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckMapEqual(obj, msg);
    }
}

TEST(MessageToStruct, Oneof) {
    {
        messages::Oneof msg;
        msg.set_f1(1001);

        auto obj = MessageToStruct<structs::Oneof>(msg);

        CheckOneofEqual(obj, msg);
    }

    {
        messages::Oneof msg;
        structs::Oneof obj;

        CheckOneofEqual(obj, msg);

        msg.set_f2("test");
        obj.test_oneof.Set<0>(1);

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckOneofEqual(obj, msg);

        msg.set_f3(messages::TestEnum::TEST_ENUM_UNSPECIFIED);

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckOneofEqual(obj, msg);

        msg.mutable_f4()->set_f1(1001);

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckOneofEqual(obj, msg);
    }
}

TEST(MessageToStruct, Indirect) {
    {
        messages::Indirect msg;
        msg.mutable_f1()->set_f1(1);
        *msg.mutable_f2() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(123'456'789'987'654'321LL);
        msg.mutable_f3()->Add()->set_f1(3);
        msg.mutable_f3()->Add()->set_f1(4);
        (*msg.mutable_f4())[1].set_f1(5);
        (*msg.mutable_f4())[2].set_f1(6);
        msg.mutable_f5()->set_f1(7);
        msg.set_f7(1001);
        msg.add_f8(messages::TEST_ENUM_UNSPECIFIED);
        (*msg.mutable_f9())["3"].set_f1(8);

        auto obj = MessageToStruct<structs::Indirect>(msg);

        CheckIndirectEqual(obj, msg);
    }

    {
        messages::Indirect msg;
        structs::Indirect obj;

        CheckIndirectEqual(obj, msg);

        msg.mutable_f1()->set_f1(10);
        msg.mutable_f3()->Add()->set_f1(11);
        msg.set_f6("hello");
        obj.f1 = {.f1 = 1001};
        obj.f4 = {{10, structs::Simple{.f1 = 1002}}};
        obj.test_oneof.Set<0>(structs::Simple{.f1 = 30});

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckIndirectEqual(obj, msg);

        msg.mutable_f5()->set_f1(12);
        msg.add_f8(messages::TEST_ENUM_VALUE2);
        msg.add_f8(messages::TEST_ENUM_VALUE1);
        obj.test_oneof.Set<0>(structs::Simple{.f1 = 1003});

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckIndirectEqual(obj, msg);
    }
}

TEST(MessageToStruct, Strong) {
    {
        messages::Strong msg;
        msg.set_f1(1001);
        msg.set_f2("hello");
        msg.add_f3(messages::TestEnum::TEST_ENUM_VALUE1);
        msg.add_f3(messages::TestEnum::TEST_ENUM_VALUE2);
        (*msg.mutable_f4())[1].set_f1(1002);
        (*msg.mutable_f4())[2].set_f1(1003);
        *msg.mutable_f5() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(123'456'789'987'654'321LL);

        auto obj = MessageToStruct<structs::Strong>(msg);

        CheckStrongEqual(obj, msg);
    }

    {
        messages::Strong msg;
        structs::Strong obj;

        CheckStrongEqual(obj, msg);

        msg.set_f1(10);
        *msg.mutable_f5() = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(-123'456'789'987'654'321LL);
        obj.f1 = structs::Strong::F1Strong{5};
        obj.f4[100] = structs::Strong::F4Strong{structs::Simple{.f1 = 6}};

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckStrongEqual(obj, msg);

        msg.set_f2("test");
        obj.test_oneof.Set<0>(structs::Strong::F5Strong{std::chrono::nanoseconds{123'456'789'987'654'321}});

        ASSERT_NO_THROW(MessageToStruct(msg, obj));
        CheckStrongEqual(obj, msg);
    }
}

TEST(MessageToStruct, Erroneous) {
    {
        messages::Erroneous msg;
        structs::Erroneous obj;

        EXPECT_NO_THROW(obj = MessageToStruct<structs::Erroneous>(msg));
    }

    {
        messages::Erroneous msg;
        msg.mutable_f1()->set_error_type(messages::ConversionFailure::TYPE_EXCEPTION);

        try {
            auto result = MessageToStruct<structs::Erroneous>(msg);
            FAIL() << "exception should be thrown";
        } catch (const ups::ReadError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Erroneous.f1'"));
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conversion_failure_exception"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }

    {
        messages::Erroneous msg;
        msg.mutable_f2()->Add()->set_error_type(messages::ConversionFailure::TYPE_ERROR);

        try {
            auto result = MessageToStruct<structs::Erroneous>(msg);
            FAIL() << "exception should be thrown";
        } catch (const ups::ReadError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Erroneous.f2.error_field'"));
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conversion_failure_error"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }

    {
        messages::Erroneous msg;
        (*msg.mutable_f3())[10].set_error_type(messages::ConversionFailure::TYPE_ERROR_WITH_UNKNOWN_FIELD);

        try {
            auto result = MessageToStruct<structs::Erroneous>(msg);
            FAIL() << "exception should be thrown";
        } catch (const ups::ReadError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Erroneous.f3.<unknown_1001>'"));
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conversion_failure_error_with_unknown_field"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }

    {
        messages::Erroneous msg;
        msg.mutable_f4()->set_error_type(messages::ConversionFailure::TYPE_ERROR_WITH_UNKNOWN_FIELD);

        try {
            auto result = MessageToStruct<structs::Erroneous>(msg);
            FAIL() << "exception should be thrown";
        } catch (const ups::ReadError& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("'messages.Erroneous.f4.<unknown_1001>'"));
            EXPECT_THAT(e.what(), ::testing::HasSubstr("conversion_failure_error_with_unknown_field"));
        } catch (...) {
            FAIL() << "unexpected exception type";
        }
    }
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
