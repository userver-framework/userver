#include "utils.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <fmt/format.h>
#include <google/protobuf/util/json_util.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

template <typename T>
using SupportsCheckMessageEqual = decltype(CheckMessageEqual(std::declval<const T&>(), std::declval<const T&>()));

template <typename T>
inline constexpr bool kSupportsCheckMessageEqual = meta::IsDetected<SupportsCheckMessageEqual, T>;

template <typename T>
void AreProtobufRepeatedEqual(const T& lhs, const T& rhs) {
    ASSERT_EQ(lhs.size(), rhs.size());

    for (int i = 0; i < lhs.size(); ++i) {
        if constexpr (kSupportsCheckMessageEqual<typename T::value_type>) {
            CheckMessageEqual(lhs[i], rhs[i]);
        } else {
            EXPECT_EQ(lhs[i], rhs[i]);
        }
    }
}

template <typename T>
void AreProtobufMapsEqual(const T& lhs, const T& rhs) {
    EXPECT_EQ(lhs.size(), rhs.size());

    for (const auto& [key, val] : lhs) {
        ASSERT_TRUE(rhs.contains(key));

        if constexpr (kSupportsCheckMessageEqual<typename T::mapped_type>) {
            CheckMessageEqual(val, rhs.at(key));
        } else {
            EXPECT_EQ(val, rhs.at(key));
        }
    }
}

formats::json::Value PrepareJsonTestData(const std::string& json_data) { return formats::json::FromString(json_data); }

formats::json::Value CreateSampleJson(const ::google::protobuf::Message& message, const PrintOptions& options) {
    using StringType = std::decay_t<decltype(message.DebugString())>;

    StringType result;
    ::google::protobuf::util::JsonPrintOptions native_options;
#if GOOGLE_PROTOBUF_VERSION >= 5026001
    native_options.always_print_fields_with_no_presence = options.always_print_fields_with_no_presence;
#else
    native_options.always_print_primitive_fields = options.always_print_fields_with_no_presence;
#endif
    native_options.always_print_enums_as_ints = options.always_print_enums_as_ints;
    native_options.preserve_proto_field_names = options.preserve_proto_field_names;

    auto status = ::google::protobuf::util::MessageToJsonString(message, &result, native_options);

    if (status.ok()) {
        return formats::json::FromString(result);
    } else {
        throw SampleError(fmt::format("Failed to create sample JSON from protobuf message: {}", status.message()));
    }
}

void InitSampleMessage(const std::string& json, ::google::protobuf::Message& message, const ParseOptions& options) {
    ::google::protobuf::util::ParseOptions native_options;
    native_options.ignore_unknown_fields = options.ignore_unknown_fields;

    auto status = ::google::protobuf::util::JsonStringToMessage(json, &message, native_options);

    if (!status.ok()) {
        throw SampleError(fmt::format("Failed to initialize sample message from JSON: {}", status.message()));
    }
}

::google::protobuf::Value ProtoValueToNative(const ProtoValue& data) {
    struct Visitor {
        ::google::protobuf::Value result;

        ::google::protobuf::Value operator()(std::monostate) { return result; }

        ::google::protobuf::Value operator()(ProtoNullValue) {
            result.set_null_value(::google::protobuf::NULL_VALUE);
            return result;
        }

        ::google::protobuf::Value operator()(double val) {
            result.set_number_value(val);
            return result;
        }

        ::google::protobuf::Value operator()(const std::string& val) {
            result.set_string_value(val);
            return result;
        }

        ::google::protobuf::Value operator()(bool val) {
            result.set_bool_value(val);
            return result;
        }

        ::google::protobuf::Value operator()(const std::vector<double>& val) {
            auto& arr = *result.mutable_list_value();

            for (const auto& item : val) {
                arr.add_values()->set_number_value(item);
            }

            return result;
        }

        ::google::protobuf::Value operator()(const std::map<std::string, std::string>& val) {
            auto& fields = *result.mutable_struct_value()->mutable_fields();

            for (const auto& [key, value] : val) {
                fields[key].set_string_value(value);
            }

            return result;
        }
    };

    return std::visit(Visitor{}, data);
}

proto_json::messages::Int32Message PrepareTestData(const Int32MessageData& message_data) {
    proto_json::messages::Int32Message message;
    message.set_field1(message_data.field1);
    message.set_field2(message_data.field2);
    message.set_field3(message_data.field3);
    return message;
}

void CheckMessageEqual(const proto_json::messages::Int32Message& lhs, const proto_json::messages::Int32Message& rhs) {
    EXPECT_EQ(lhs.field1(), rhs.field1());
    EXPECT_EQ(lhs.field2(), rhs.field2());
    EXPECT_EQ(lhs.field3(), rhs.field3());
}

proto_json::messages::UInt32Message PrepareTestData(const UInt32MessageData& message_data) {
    proto_json::messages::UInt32Message message;
    message.set_field1(message_data.field1);
    message.set_field2(message_data.field2);
    return message;
}

void CheckMessageEqual(const proto_json::messages::UInt32Message& lhs, const proto_json::messages::UInt32Message& rhs) {
    EXPECT_EQ(lhs.field1(), rhs.field1());
    EXPECT_EQ(lhs.field2(), rhs.field2());
}

proto_json::messages::Int64Message PrepareTestData(const Int64MessageData& message_data) {
    proto_json::messages::Int64Message message;
    message.set_field1(message_data.field1);
    message.set_field2(message_data.field2);
    message.set_field3(message_data.field3);
    return message;
}

void CheckMessageEqual(const proto_json::messages::Int64Message& lhs, const proto_json::messages::Int64Message& rhs) {
    EXPECT_EQ(lhs.field1(), rhs.field1());
    EXPECT_EQ(lhs.field2(), rhs.field2());
    EXPECT_EQ(lhs.field3(), rhs.field3());
}

proto_json::messages::UInt64Message PrepareTestData(const UInt64MessageData& message_data) {
    proto_json::messages::UInt64Message message;
    message.set_field1(message_data.field1);
    message.set_field2(message_data.field2);
    return message;
}

void CheckMessageEqual(const proto_json::messages::UInt64Message& lhs, const proto_json::messages::UInt64Message& rhs) {
    EXPECT_EQ(lhs.field1(), rhs.field1());
    EXPECT_EQ(lhs.field2(), rhs.field2());
}

proto_json::messages::FloatMessage PrepareTestData(const FloatMessageData& message_data) {
    proto_json::messages::FloatMessage message;
    message.set_field1(message_data.field1);
    return message;
}

void CheckMessageEqual(const proto_json::messages::FloatMessage& lhs, const proto_json::messages::FloatMessage& rhs) {
    if (!std::isnan(lhs.field1())) {
        EXPECT_EQ(lhs.field1(), rhs.field1());
    } else {
        EXPECT_TRUE(std::isnan(rhs.field1()));
    }
}

proto_json::messages::DoubleMessage PrepareTestData(const DoubleMessageData& message_data) {
    proto_json::messages::DoubleMessage message;
    message.set_field1(message_data.field1);
    return message;
}

void CheckMessageEqual(const proto_json::messages::DoubleMessage& lhs, const proto_json::messages::DoubleMessage& rhs) {
    if (!std::isnan(lhs.field1())) {
        EXPECT_EQ(lhs.field1(), rhs.field1());
    } else {
        EXPECT_TRUE(std::isnan(rhs.field1()));
    }
}

proto_json::messages::BoolMessage PrepareTestData(const BoolMessageData& message_data) {
    proto_json::messages::BoolMessage message;
    message.set_field1(message_data.field1);
    return message;
}

void CheckMessageEqual(const proto_json::messages::BoolMessage& lhs, const proto_json::messages::BoolMessage& rhs) {
    EXPECT_EQ(lhs.field1(), rhs.field1());
}

proto_json::messages::StringMessage PrepareTestData(const StringMessageData& message_data) {
    proto_json::messages::StringMessage message;
    message.set_field1(message_data.field1);
    return message;
}

void CheckMessageEqual(const proto_json::messages::StringMessage& lhs, const proto_json::messages::StringMessage& rhs) {
    EXPECT_EQ(lhs.field1(), rhs.field1());
}

proto_json::messages::BytesMessage PrepareTestData(const BytesMessageData& message_data) {
    proto_json::messages::BytesMessage message;
    message.set_field1(message_data.field1);
    return message;
}

void CheckMessageEqual(const proto_json::messages::BytesMessage& lhs, const proto_json::messages::BytesMessage& rhs) {
    EXPECT_EQ(lhs.field1(), rhs.field1());
}

proto_json::messages::EnumMessage PrepareTestData(const EnumMessageData& message_data) {
    proto_json::messages::EnumMessage message;
    message.set_field1(message_data.field1);
    return message;
}

void CheckMessageEqual(const proto_json::messages::EnumMessage& lhs, const proto_json::messages::EnumMessage& rhs) {
    EXPECT_EQ(lhs.field1(), rhs.field1());
}

proto_json::messages::NestedMessage PrepareTestData(const NestedMessageData& message_data) {
    proto_json::messages::NestedMessage message;
    if (message_data.field1) {
        message.mutable_parent()->set_field1(message_data.field1.value());
    }
    return message;
}

void CheckMessageEqual(const proto_json::messages::NestedMessage& lhs, const proto_json::messages::NestedMessage& rhs) {
    EXPECT_TRUE((lhs.has_parent() && rhs.has_parent()) || (!lhs.has_parent() && !rhs.has_parent()));
    EXPECT_EQ(lhs.parent().field1(), rhs.parent().field1());
}

proto_json::messages::WrapperMessage PrepareTestData(const WrapperMessageData& message_data) {
    proto_json::messages::WrapperMessage message;

    if (message_data.field1) {
        message.mutable_field1()->set_value(message_data.field1.value());
    }

    if (message_data.field2) {
        message.mutable_field2()->set_value(message_data.field2.value());
    }

    if (message_data.field3) {
        message.mutable_field3()->set_value(message_data.field3.value());
    }

    if (message_data.field4) {
        message.mutable_field4()->set_value(message_data.field4.value());
    }

    if (message_data.field5) {
        message.mutable_field5()->set_value(message_data.field5.value());
    }

    if (message_data.field6) {
        message.mutable_field6()->set_value(message_data.field6.value());
    }

    if (message_data.field7) {
        message.mutable_field7()->set_value(message_data.field7.value());
    }

    if (message_data.field8) {
        message.mutable_field8()->set_value(message_data.field8.value());
    }

    if (message_data.field9) {
        message.mutable_field9()->set_value(message_data.field9.value());
    }
    return message;
}

void CheckMessageEqual(
    const proto_json::messages::WrapperMessage& lhs,
    const proto_json::messages::WrapperMessage& rhs
) {
    EXPECT_TRUE((lhs.has_field1() && rhs.has_field1()) || (!lhs.has_field1() && !rhs.has_field1()));
    EXPECT_TRUE((lhs.has_field2() && rhs.has_field2()) || (!lhs.has_field2() && !rhs.has_field2()));
    EXPECT_TRUE((lhs.has_field3() && rhs.has_field3()) || (!lhs.has_field3() && !rhs.has_field3()));
    EXPECT_TRUE((lhs.has_field4() && rhs.has_field4()) || (!lhs.has_field4() && !rhs.has_field4()));
    EXPECT_TRUE((lhs.has_field5() && rhs.has_field5()) || (!lhs.has_field5() && !rhs.has_field5()));
    EXPECT_TRUE((lhs.has_field6() && rhs.has_field6()) || (!lhs.has_field6() && !rhs.has_field6()));
    EXPECT_TRUE((lhs.has_field7() && rhs.has_field7()) || (!lhs.has_field7() && !rhs.has_field7()));
    EXPECT_TRUE((lhs.has_field8() && rhs.has_field8()) || (!lhs.has_field8() && !rhs.has_field8()));
    EXPECT_TRUE((lhs.has_field9() && rhs.has_field9()) || (!lhs.has_field9() && !rhs.has_field9()));

    EXPECT_EQ(lhs.field1().value(), rhs.field1().value());
    EXPECT_EQ(lhs.field2().value(), rhs.field2().value());
    EXPECT_EQ(lhs.field3().value(), rhs.field3().value());
    EXPECT_EQ(lhs.field4().value(), rhs.field4().value());
    EXPECT_EQ(lhs.field5().value(), rhs.field5().value());
    EXPECT_EQ(lhs.field6().value(), rhs.field6().value());
    EXPECT_EQ(lhs.field7().value(), rhs.field7().value());
    EXPECT_EQ(lhs.field8().value(), rhs.field8().value());
    EXPECT_EQ(lhs.field9().value(), rhs.field9().value());
}

proto_json::messages::FieldMaskMessage PrepareTestData(const FieldMaskMessageData& message_data) {
    proto_json::messages::FieldMaskMessage message;

    if (!message_data.field1) {
        return message;
    }

    (void)message.mutable_field1();

    for (const auto& path : message_data.field1.value()) {
        message.mutable_field1()->add_paths(path);
    }

    return message;
}

void CheckMessageEqual(const ::google::protobuf::FieldMask& lhs, const ::google::protobuf::FieldMask& rhs) {
    std::vector<std::string> mask_lhs, mask_rhs;

    for (const auto& path : lhs.paths()) {
        mask_lhs.push_back(path);
    }

    for (const auto& path : rhs.paths()) {
        mask_rhs.push_back(path);
    }

    std::sort(mask_lhs.begin(), mask_lhs.end());
    std::sort(mask_rhs.begin(), mask_rhs.end());

    EXPECT_EQ(mask_lhs, mask_rhs);
}

void CheckMessageEqual(
    const proto_json::messages::FieldMaskMessage& lhs,
    const proto_json::messages::FieldMaskMessage& rhs
) {
    EXPECT_TRUE((lhs.has_field1() && rhs.has_field1()) || (!lhs.has_field1() && !rhs.has_field1()));
    CheckMessageEqual(lhs.field1(), rhs.field1());
}

proto_json::messages::DurationMessage PrepareTestData(const DurationMessageData& message_data) {
    proto_json::messages::DurationMessage message;

    if (!message_data.do_not_set) {
        message.mutable_field1()->set_seconds(message_data.seconds);
        message.mutable_field1()->set_nanos(message_data.nanos);
    }

    return message;
}

void CheckMessageEqual(const ::google::protobuf::Duration& lhs, const ::google::protobuf::Duration& rhs) {
    EXPECT_EQ(lhs.seconds(), rhs.seconds());
    EXPECT_EQ(lhs.nanos(), rhs.nanos());
}

void CheckMessageEqual(
    const proto_json::messages::DurationMessage& lhs,
    const proto_json::messages::DurationMessage& rhs
) {
    EXPECT_TRUE((lhs.has_field1() && rhs.has_field1()) || (!lhs.has_field1() && !rhs.has_field1()));
    CheckMessageEqual(lhs.field1(), rhs.field1());
}

proto_json::messages::TimestampMessage PrepareTestData(const TimestampMessageData& message_data) {
    proto_json::messages::TimestampMessage message;

    if (!message_data.do_not_set) {
        message.mutable_field1()->set_seconds(message_data.seconds);
        message.mutable_field1()->set_nanos(message_data.nanos);
    }

    return message;
}

void CheckMessageEqual(const ::google::protobuf::Timestamp& lhs, const ::google::protobuf::Timestamp& rhs) {
    EXPECT_EQ(lhs.seconds(), rhs.seconds());
    EXPECT_EQ(lhs.nanos(), rhs.nanos());
}

void CheckMessageEqual(
    const proto_json::messages::TimestampMessage& lhs,
    const proto_json::messages::TimestampMessage& rhs
) {
    EXPECT_TRUE((lhs.has_field1() && rhs.has_field1()) || (!lhs.has_field1() && !rhs.has_field1()));
    CheckMessageEqual(lhs.field1(), rhs.field1());
}

proto_json::messages::ValueMessage PrepareTestData(const ValueMessageData& message_data) {
    struct Visitor {
        proto_json::messages::ValueMessage result;

        proto_json::messages::ValueMessage operator()(const ProtoValue& val) {
            *result.mutable_field1() = ProtoValueToNative(val);
            return result;
        }

        proto_json::messages::ValueMessage operator()(const std::vector<ProtoValue>& val) {
            auto& arr = *result.mutable_field1()->mutable_list_value();

            for (const auto& item : val) {
                *arr.add_values() = ProtoValueToNative(item);
            }

            return result;
        }

        proto_json::messages::ValueMessage operator()(const std::map<std::string, ProtoValue>& val) {
            auto& fields = *result.mutable_field1()->mutable_struct_value()->mutable_fields();

            for (const auto& [key, value] : val) {
                fields[key] = ProtoValueToNative(value);
            }

            return result;
        }
    };

    if (message_data.field1) {
        return std::visit(Visitor{}, *message_data.field1);
    } else {
        return proto_json::messages::ValueMessage{};
    }
}

void CheckMessageEqual(const ::google::protobuf::Value& lhs, const ::google::protobuf::Value& rhs) {
    if (lhs.has_null_value()) {
        ASSERT_TRUE(rhs.has_null_value());
        EXPECT_EQ(lhs.null_value(), rhs.null_value());
    } else if (lhs.has_number_value()) {
        ASSERT_TRUE(rhs.has_number_value());
        EXPECT_EQ(lhs.number_value(), rhs.number_value());
    } else if (lhs.has_string_value()) {
        ASSERT_TRUE(rhs.has_string_value());
        EXPECT_EQ(lhs.string_value(), rhs.string_value());
    } else if (lhs.has_bool_value()) {
        ASSERT_TRUE(rhs.has_bool_value());
        EXPECT_EQ(lhs.bool_value(), rhs.bool_value());
    } else if (lhs.has_list_value()) {
        ASSERT_TRUE(rhs.has_list_value());
        CheckMessageEqual(lhs.list_value(), rhs.list_value());
    } else if (lhs.has_struct_value()) {
        ASSERT_TRUE(rhs.has_struct_value());
        CheckMessageEqual(lhs.struct_value(), rhs.struct_value());
    } else {
        EXPECT_TRUE(
            !rhs.has_null_value() && !rhs.has_number_value() && !rhs.has_string_value() && !rhs.has_bool_value() &&
            !rhs.has_list_value() && !rhs.has_struct_value()
        );
    }
}

void CheckMessageEqual(const ::google::protobuf::ListValue& lhs, const ::google::protobuf::ListValue& rhs) {
    AreProtobufRepeatedEqual(lhs.values(), rhs.values());
}

void CheckMessageEqual(const ::google::protobuf::Struct& lhs, const ::google::protobuf::Struct& rhs) {
    AreProtobufMapsEqual(lhs.fields(), rhs.fields());
}

void CheckMessageEqual(const proto_json::messages::ValueMessage& lhs, const proto_json::messages::ValueMessage& rhs) {
    EXPECT_TRUE((lhs.has_field1() && rhs.has_field1()) || (!lhs.has_field1() && !rhs.has_field1()));
    CheckMessageEqual(lhs.field1(), rhs.field1());
}

template <typename T>
void CheckAnyPayloadEqual(const ::google::protobuf::Any& lhs, const ::google::protobuf::Any& rhs) {
    ASSERT_TRUE(lhs.Is<T>());
    ASSERT_TRUE(rhs.Is<T>());

    T lhs_payload, rhs_payload;

    ASSERT_TRUE(lhs.UnpackTo(&lhs_payload));
    ASSERT_TRUE(rhs.UnpackTo(&rhs_payload));

    CheckMessageEqual(lhs_payload, rhs_payload);
}

proto_json::messages::AnyMessage PrepareTestData(const AnyMessageData& message_data) {
    struct Visitor {
        ::google::protobuf::Any result;

        ::google::protobuf::Any operator()(const Int32MessageData& payload) {
            auto msg = PrepareTestData(payload);

            if (!result.PackFrom(msg)) {
                throw std::runtime_error("Failed to create 'google.protobuf.Any'");
            }

            return result;
        }

        ::google::protobuf::Any operator()(const DurationMessageData& payload) {
            const auto msg = PrepareTestData(payload);

            if (!result.PackFrom(msg.field1())) {
                throw std::runtime_error("Failed to create 'google.protobuf.Any'");
            }

            return result;
        }

        ::google::protobuf::Any operator()(const ValueMessageData& payload) {
            const auto msg = PrepareTestData(payload);

            if (!result.PackFrom(msg.field1())) {
                throw std::runtime_error("Failed to create 'google.protobuf.Any'");
            }

            return result;
        }

        ::google::protobuf::Any operator()(const RawAnyData& payload) {
            result.set_type_url(payload.type_url);
            result.set_value(payload.value);
            return result;
        }
    };

    proto_json::messages::AnyMessage result;

    if (!message_data.field1) {
        return result;
    }

    auto any_message = std::visit(Visitor{}, message_data.field1.value());

    if (!message_data.add_nesting) {
        *result.mutable_field1() = std::move(any_message);
    } else {
        ::google::protobuf::Any top;

        if (!top.PackFrom(any_message)) {
            throw std::runtime_error("Failed to create top-level 'google.protobuf.Any'");
        }

        *result.mutable_field1() = std::move(top);
    }

    return result;
}

void CheckMessageEqual(const ::google::protobuf::Any& lhs, const ::google::protobuf::Any& rhs) {
    if (lhs.Is<proto_json::messages::Int32Message>()) {
        CheckAnyPayloadEqual<proto_json::messages::Int32Message>(lhs, rhs);
    } else if (lhs.Is<::google::protobuf::Duration>()) {
        CheckAnyPayloadEqual<::google::protobuf::Duration>(lhs, rhs);
    } else if (lhs.Is<::google::protobuf::Value>()) {
        CheckAnyPayloadEqual<::google::protobuf::Value>(lhs, rhs);
    } else if (lhs.Is<::google::protobuf::Any>()) {
        CheckAnyPayloadEqual<::google::protobuf::Any>(lhs, rhs);
    } else {
        EXPECT_EQ(lhs.type_url(), rhs.type_url());
        EXPECT_EQ(lhs.value(), rhs.value());
    }
}

void CheckMessageEqual(const proto_json::messages::AnyMessage& lhs, const proto_json::messages::AnyMessage& rhs) {
    EXPECT_TRUE((lhs.has_field1() && rhs.has_field1()) || (!lhs.has_field1() && !rhs.has_field1()));

    if (lhs.has_field1()) {
        CheckMessageEqual(lhs.field1(), rhs.field1());
    }
}

proto_json::messages::OneofMessage PrepareTestData(const OneofMessageData& message_data) {
    proto_json::messages::OneofMessage result;

    if (message_data.field1) {
        result.set_field1(message_data.field1.value());
    } else if (message_data.field2) {
        result.set_field2(message_data.field2.value());
    }

    return result;
}

void CheckMessageEqual(const proto_json::messages::OneofMessage& lhs, const proto_json::messages::OneofMessage& rhs) {
    if (lhs.has_field1()) {
        EXPECT_TRUE(rhs.has_field1());
        EXPECT_EQ(lhs.field1(), rhs.field1());
    } else if (lhs.has_field2()) {
        EXPECT_TRUE(rhs.has_field2());
        EXPECT_EQ(lhs.field2(), rhs.field2());
    } else {
        EXPECT_FALSE(rhs.has_field1() || rhs.has_field2());
    }
}

proto_json::messages::RepeatedMessage PrepareTestData(const RepeatedMessageData& message_data) {
    proto_json::messages::RepeatedMessage result;

    for (const auto& item : message_data.field1) {
        result.add_field1(item);
    }

    for (const auto& item : message_data.field2) {
        *result.add_field2() = PrepareTestData(item);
    }

    for (const auto& item : message_data.field3) {
        *result.add_field3() = PrepareTestData(item).field1();
    }

    return result;
}

void CheckMessageEqual(
    const proto_json::messages::RepeatedMessage& lhs,
    const proto_json::messages::RepeatedMessage& rhs
) {
    AreProtobufRepeatedEqual(lhs.field1(), rhs.field1());
    AreProtobufRepeatedEqual(lhs.field2(), rhs.field2());
    AreProtobufRepeatedEqual(lhs.field3(), rhs.field3());
}

proto_json::messages::MapMessage PrepareTestData(const MapMessageData& message_data) {
    proto_json::messages::MapMessage result;

    for (const auto& [key, val] : message_data.field1) {
        (*result.mutable_field1())[key] = val;
    }

    for (const auto& [key, val] : message_data.field2) {
        (*result.mutable_field2())[key] = val;
    }

    for (const auto& [key, val] : message_data.field3) {
        (*result.mutable_field3())[key] = val;
    }

    for (const auto& [key, val] : message_data.field4) {
        (*result.mutable_field4())[key] = val;
    }

    for (const auto& [key, val] : message_data.field5) {
        (*result.mutable_field5())[key] = val;
    }

    for (const auto& [key, val] : message_data.field6) {
        (*result.mutable_field6())[key] = PrepareTestData(val);
    }

    for (const auto& [key, val] : message_data.field7) {
        (*result.mutable_field7())[key] = PrepareTestData(val).field1();
    }

    return result;
}

void CheckMessageEqual(const proto_json::messages::MapMessage& lhs, const proto_json::messages::MapMessage& rhs) {
    AreProtobufMapsEqual(lhs.field1(), rhs.field1());
    AreProtobufMapsEqual(lhs.field2(), rhs.field2());
    AreProtobufMapsEqual(lhs.field3(), rhs.field3());
    AreProtobufMapsEqual(lhs.field4(), rhs.field4());
    AreProtobufMapsEqual(lhs.field5(), rhs.field5());
    AreProtobufMapsEqual(lhs.field6(), rhs.field6());
    AreProtobufMapsEqual(lhs.field7(), rhs.field7());
}

proto_json::messages::NullValueMessage PrepareTestData(const NullValueMessageData& message_data) {
    proto_json::messages::NullValueMessage result;
    result.set_field1(message_data.field1);

    if (message_data.field2) {
        result.set_field2(message_data.field2.value());
    }

    for (const auto& item : message_data.field3) {
        result.add_field3(item);
    }

    for (const auto& [key, val] : message_data.field4) {
        (*result.mutable_field4())[key] = val;
    }

    return result;
}

void CheckMessageEqual(
    const proto_json::messages::NullValueMessage& lhs,
    const proto_json::messages::NullValueMessage& rhs
) {
    EXPECT_EQ(lhs.field1(), rhs.field1());

    if (lhs.has_field2()) {
        EXPECT_TRUE(rhs.has_field2());
        EXPECT_EQ(lhs.field2(), rhs.field2());
    } else {
        EXPECT_FALSE(rhs.has_field2());
    }

    AreProtobufRepeatedEqual(lhs.field3(), rhs.field3());
    AreProtobufMapsEqual(lhs.field4(), rhs.field4());
}

proto_json::messages::PresenceMessage PrepareTestData(const PresenceMessageData& message_data) {
    proto_json::messages::PresenceMessage result;

    result.set_field1(message_data.field1);

    if (message_data.field2) {
        result.set_field2(message_data.field2.value());
    }

    if (message_data.field3) {
        result.mutable_field3()->set_field1(message_data.field3->field1);
        result.mutable_field3()->set_field2(message_data.field3->field2);
        result.mutable_field3()->set_field3(message_data.field3->field3);
    }

    if (!message_data.field4.empty()) {
        for (const auto& val : message_data.field4) {
            result.add_field4(val);
        }
    }

    if (!message_data.field5.empty()) {
        for (const auto& [key, val] : message_data.field5) {
            (*result.mutable_field5())[key] = val;
        }
    }

    return result;
}

void CheckMessageEqual(
    const proto_json::messages::PresenceMessage& lhs,
    const proto_json::messages::PresenceMessage& rhs
) {
    EXPECT_EQ(lhs.field1(), rhs.field1());

    if (lhs.has_field2()) {
        EXPECT_TRUE(rhs.has_field2());
        EXPECT_EQ(lhs.field2(), rhs.field2());
    } else {
        EXPECT_FALSE(rhs.has_field2());
    }

    if (lhs.has_field3()) {
        EXPECT_TRUE(rhs.has_field3());
        CheckMessageEqual(lhs.field3(), rhs.field3());
    } else {
        EXPECT_FALSE(rhs.has_field3());
    }

    AreProtobufRepeatedEqual(lhs.field4(), rhs.field4());
    AreProtobufMapsEqual(lhs.field5(), rhs.field5());
}

proto_json::messages::UnknownFieldMessage PrepareTestData(const UnknownFieldMessageData& message_data) {
    proto_json::messages::UnknownFieldMessage result;

    *result.mutable_field1() = PrepareTestData(message_data.field1);

    for (const auto& item : message_data.field2) {
        *result.add_field2() = PrepareTestData(item);
    }

    for (const auto& [key, val] : message_data.field3) {
        (*result.mutable_field3())[key] = PrepareTestData(val);
    }

    return result;
}

void CheckMessageEqual(
    const proto_json::messages::UnknownFieldMessage& lhs,
    const proto_json::messages::UnknownFieldMessage& rhs
) {
    EXPECT_TRUE((lhs.has_field1() && rhs.has_field1()) || (!lhs.has_field1() && !rhs.has_field1()));
    CheckMessageEqual(lhs.field1(), rhs.field1());

    AreProtobufRepeatedEqual(lhs.field2(), rhs.field2());
    AreProtobufMapsEqual(lhs.field3(), rhs.field3());
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
