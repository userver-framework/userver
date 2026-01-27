#include "structs.hpp"

#include <gtest/gtest.h>

#include <google/protobuf/util/time_util.h>

#include <userver/proto-structs/io/impl/read.hpp>
#include <userver/proto-structs/io/impl/write.hpp>
#include <userver/proto-structs/io/supported_types_conv.hpp>

#include "messages.pb.h"

namespace structs {

namespace {

void CheckJsonValue(const formats::json::Value& json, const ::google::protobuf::Value& msg);
void CheckJsonValue(const formats::json::Value& json, const ::google::protobuf::ListValue& msg);
void CheckJsonValue(const formats::json::Value& json, const ::google::protobuf::Struct& msg);

void CheckJsonValue(const formats::json::Value& json, const ::google::protobuf::Value& msg) {
    if (msg.has_null_value()) {
        ASSERT_TRUE(json.IsNull());
    } else if (msg.has_number_value()) {
        ASSERT_TRUE(json.IsNumber());
        EXPECT_EQ(json.As<double>(), msg.number_value());
    } else if (msg.has_string_value()) {
        ASSERT_TRUE(json.IsString());
        EXPECT_EQ(json.As<std::string>(), msg.string_value());
    } else if (msg.has_bool_value()) {
        ASSERT_TRUE(json.IsBool());
        EXPECT_EQ(json.As<bool>(), msg.bool_value());
    } else if (msg.has_list_value()) {
        CheckJsonValue(json, msg.list_value());
    } else if (msg.has_struct_value()) {
        CheckJsonValue(json, msg.struct_value());
    } else {
        ADD_FAILURE() << "incomparable 'google::protobuf::Value'";
    }
}

void CheckJsonValue(const formats::json::Value& json, const ::google::protobuf::ListValue& msg) {
    ASSERT_TRUE(json.IsArray());
    ASSERT_EQ(static_cast<int>(json.GetSize()), msg.values().size());

    for (int i = 0; i < msg.values().size(); ++i) {
        CheckJsonValue(json[i], msg.values()[i]);
    }
}

void CheckJsonValue(const formats::json::Value& json, const ::google::protobuf::Struct& msg) {
    ASSERT_TRUE(json.IsObject());
    EXPECT_EQ(json.GetSize(), msg.fields().size());

    for (const auto& [key, val] : msg.fields()) {
        ASSERT_TRUE(json.HasMember(key));
        CheckJsonValue(json[key], val);
    }
}

}  // namespace

ConversionFailure ReadProtoStruct(
    ups::io::ReadContext& ctx,
    ups::io::To<ConversionFailure>,
    const messages::ConversionFailure& msg
) {
    if (msg.error_type() == messages::ConversionFailure::TYPE_EXCEPTION) {
        throw std::runtime_error("conversion_failure_exception");
    } else if (msg.error_type() == messages::ConversionFailure::TYPE_ERROR) {
        ctx.AddError(messages::ConversionFailure::kErrorFieldFieldNumber, "conversion_failure_error");
    } else {
        ctx.AddError(1001, "conversion_failure_error_with_unknown_field");
    }

    return {};
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const ConversionFailure& obj, messages::ConversionFailure&) {
    if (obj.error_type == ConversionFailureType::kException) {
        throw std::runtime_error("conversion_failure_exception");
    } else if (obj.error_type == ConversionFailureType::kError) {
        ctx.AddError(messages::ConversionFailure::kErrorFieldFieldNumber, "conversion_failure_error");
    } else {
        ctx.AddError(1001, "conversion_failure_error_with_unknown_field");
    }
}

Scalar ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Scalar>, const messages::Scalar& msg) {
    return {
        .f1 = ups::io::impl::ReadField<
            bool>(ctx, ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF1FieldNumber, &messages::Scalar::f1)),
        .f2 = ups::io::impl::ReadField<int32_t>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF2FieldNumber, &messages::Scalar::f2)
        ),
        .f3 = ups::io::impl::ReadField<uint32_t>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF3FieldNumber, &messages::Scalar::f3)
        ),
        .f4 = ups::io::impl::ReadField<int64_t>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF4FieldNumber, &messages::Scalar::f4)
        ),
        .f5 = ups::io::impl::ReadField<uint64_t>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF5FieldNumber, &messages::Scalar::f5)
        ),
        .f6 = ups::io::impl::ReadField<
            float>(ctx, ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF6FieldNumber, &messages::Scalar::f6)),
        .f7 = ups::io::impl::ReadField<double>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF7FieldNumber, &messages::Scalar::f7)
        ),
        .f8 = ups::io::impl::ReadField<std::string>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF8FieldNumber, &messages::Scalar::f8)
        ),
        .f9 = ups::io::impl::ReadField<std::string>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF9FieldNumber, &messages::Scalar::f9)
        ),
        .f10 = ups::io::impl::ReadField<TestEnum>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF10FieldNumber, &messages::Scalar::f10)
        ),
        .f11 = ups::io::impl::ReadField<std::size_t>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Scalar::kF11FieldNumber, &messages::Scalar::f11)
        )
    };
}

template <typename T>
void WriteScalarStruct(ups::io::WriteContext& ctx, T&& obj, messages::Scalar& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF1FieldNumber,
            &messages::Scalar::set_f1,
            &messages::Scalar::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF2FieldNumber,
            &messages::Scalar::set_f2,
            &messages::Scalar::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF3FieldNumber,
            &messages::Scalar::set_f3,
            &messages::Scalar::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f4,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF4FieldNumber,
            &messages::Scalar::set_f4,
            &messages::Scalar::clear_f4
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f5,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF5FieldNumber,
            &messages::Scalar::set_f5,
            &messages::Scalar::clear_f5
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f6,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF6FieldNumber,
            &messages::Scalar::set_f6,
            &messages::Scalar::clear_f6
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f7,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF7FieldNumber,
            &messages::Scalar::set_f7,
            &messages::Scalar::clear_f7
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f8,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF8FieldNumber,
            &messages::Scalar::set_f8<const std::string&>,
            &messages::Scalar::set_f8<std::string>,
            &messages::Scalar::clear_f8
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f9,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF9FieldNumber,
            &messages::Scalar::set_f9<const std::string&>,
            &messages::Scalar::set_f9<std::string>,
            &messages::Scalar::clear_f9
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f10,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF10FieldNumber,
            &messages::Scalar::set_f10,
            &messages::Scalar::clear_f10
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f11,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Scalar::kF11FieldNumber,
            &messages::Scalar::set_f11,
            &messages::Scalar::clear_f11
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Scalar& obj, messages::Scalar& msg) {
    WriteScalarStruct(ctx, obj, msg);
}

void WriteProtoStruct(ups::io::WriteContext& ctx, Scalar&& obj, messages::Scalar& msg) {
    WriteScalarStruct(ctx, std::move(obj), msg);
}

WellKnownStd ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<WellKnownStd>, const messages::WellKnownStd& msg) {
    return {
        .f1 = ups::io::impl::ReadField<std::chrono::time_point<std::chrono::system_clock>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownStd::kF1FieldNumber, &messages::WellKnownStd::f1)
        ),
        .f2 = ups::io::impl::ReadField<std::chrono::milliseconds>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownStd::kF2FieldNumber, &messages::WellKnownStd::f2)
        ),
        .f3 = ups::io::impl::ReadField<std::chrono::year_month_day>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownStd::kF3FieldNumber, &messages::WellKnownStd::f3)
        ),
        .f4 = ups::io::impl::ReadField<std::chrono::hh_mm_ss<std::chrono::microseconds>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownStd::kF4FieldNumber, &messages::WellKnownStd::f4)
        )
    };
}

template <typename T>
void WriteWellKnownStruct(ups::io::WriteContext& ctx, T&& obj, messages::WellKnownStd& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownStd::kF1FieldNumber,
            &messages::WellKnownStd::mutable_f1,
            &messages::WellKnownStd::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownStd::kF2FieldNumber,
            &messages::WellKnownStd::mutable_f2,
            &messages::WellKnownStd::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownStd::kF3FieldNumber,
            &messages::WellKnownStd::mutable_f3,
            &messages::WellKnownStd::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f4,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownStd::kF4FieldNumber,
            &messages::WellKnownStd::mutable_f4,
            &messages::WellKnownStd::clear_f4
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const WellKnownStd& obj, messages::WellKnownStd& msg) {
    WriteWellKnownStruct(ctx, obj, msg);
}

void WriteProtoStruct(ups::io::WriteContext& ctx, WellKnownStd&& obj, messages::WellKnownStd& msg) {
    WriteWellKnownStruct(ctx, std::move(obj), msg);
}

WellKnownUsrv ReadProtoStruct(
    ups::io::ReadContext& ctx,
    ups::io::To<WellKnownUsrv>,
    const messages::WellKnownUsrv& msg
) {
    return {
        .f1 = ups::io::impl::ReadField<ups::Any>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownUsrv::kF1FieldNumber, &messages::WellKnownUsrv::f1)
        ),
        .f2 = ups::io::impl::ReadField<ups::Timestamp>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownUsrv::kF2FieldNumber, &messages::WellKnownUsrv::f2)
        ),
        .f3 = ups::io::impl::ReadField<ups::Duration>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownUsrv::kF3FieldNumber, &messages::WellKnownUsrv::f3)
        ),
        .f4 = ups::io::impl::ReadField<ups::Date>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownUsrv::kF4FieldNumber, &messages::WellKnownUsrv::f4)
        ),
        .f5 = ups::io::impl::ReadField<ups::TimeOfDay>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownUsrv::kF5FieldNumber, &messages::WellKnownUsrv::f5)
        ),
        .f6 = ups::io::impl::ReadField<USERVER_NAMESPACE::utils::datetime::TimeOfDay<std::chrono::microseconds>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownUsrv::kF6FieldNumber, &messages::WellKnownUsrv::f6)
        ),
        .f7 = ups::io::impl::ReadField<USERVER_NAMESPACE::decimal64::Decimal<3>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownUsrv::kF7FieldNumber, &messages::WellKnownUsrv::f7)
        )
    };
}

template <typename T>
void WriteWellKnownStruct(ups::io::WriteContext& ctx, T&& obj, messages::WellKnownUsrv& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownUsrv::kF1FieldNumber,
            &messages::WellKnownUsrv::mutable_f1,
            &messages::WellKnownUsrv::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownUsrv::kF2FieldNumber,
            &messages::WellKnownUsrv::mutable_f2,
            &messages::WellKnownUsrv::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownUsrv::kF3FieldNumber,
            &messages::WellKnownUsrv::mutable_f3,
            &messages::WellKnownUsrv::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f4,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownUsrv::kF4FieldNumber,
            &messages::WellKnownUsrv::mutable_f4,
            &messages::WellKnownUsrv::clear_f4
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f5,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownUsrv::kF5FieldNumber,
            &messages::WellKnownUsrv::mutable_f5,
            &messages::WellKnownUsrv::clear_f5
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f6,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownUsrv::kF6FieldNumber,
            &messages::WellKnownUsrv::mutable_f6,
            &messages::WellKnownUsrv::clear_f6
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f7,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownUsrv::kF7FieldNumber,
            &messages::WellKnownUsrv::mutable_f7,
            &messages::WellKnownUsrv::clear_f7
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const WellKnownUsrv& obj, messages::WellKnownUsrv& msg) {
    WriteWellKnownStruct(ctx, obj, msg);
}

void WriteProtoStruct(ups::io::WriteContext& ctx, WellKnownUsrv&& obj, messages::WellKnownUsrv& msg) {
    WriteWellKnownStruct(ctx, std::move(obj), msg);
}

WellKnownJson ReadProtoStruct(
    ups::io::ReadContext& ctx,
    ups::io::To<WellKnownJson>,
    const messages::WellKnownJson& msg
) {
    return {
        .f1 = ups::io::impl::ReadField<formats::json::Value>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownJson::kF1FieldNumber, &messages::WellKnownJson::f1)
        ),
        .f2 = ups::io::impl::ReadField<formats::json::Array>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownJson::kF2FieldNumber, &messages::WellKnownJson::f2)
        ),
        .f3 = ups::io::impl::ReadField<formats::json::Object>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::WellKnownJson::kF3FieldNumber, &messages::WellKnownJson::f3)
        )
    };
}

template <typename T>
void WriteWellKnownJsonStruct(ups::io::WriteContext& ctx, T&& obj, messages::WellKnownJson& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownJson::kF1FieldNumber,
            &messages::WellKnownJson::mutable_f1,
            &messages::WellKnownJson::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownJson::kF2FieldNumber,
            &messages::WellKnownJson::mutable_f2,
            &messages::WellKnownJson::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::WellKnownJson::kF3FieldNumber,
            &messages::WellKnownJson::mutable_f3,
            &messages::WellKnownJson::clear_f3
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const WellKnownJson& obj, messages::WellKnownJson& msg) {
    WriteWellKnownJsonStruct(ctx, obj, msg);
}

void WriteProtoStruct(ups::io::WriteContext& ctx, WellKnownJson&& obj, messages::WellKnownJson& msg) {
    WriteWellKnownJsonStruct(ctx, std::move(obj), msg);
}

Optional ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Optional>, const messages::Optional& msg) {
    return {
        .f1 = ups::io::impl::ReadField<std::optional<int32_t>>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Optional::kF1FieldNumber,
                &messages::Optional::f1,
                &messages::Optional::has_f1
            )
        ),
        .f2 = ups::io::impl::ReadField<std::optional<std::string>>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Optional::kF2FieldNumber,
                &messages::Optional::f2,
                &messages::Optional::has_f2
            )
        ),
        .f3 = ups::io::impl::ReadField<std::optional<TestEnum>>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Optional::kF3FieldNumber,
                &messages::Optional::f3,
                &messages::Optional::has_f3
            )
        ),
        .f4 = ups::io::impl::ReadField<std::optional<Simple>>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Optional::kF4FieldNumber,
                &messages::Optional::f4,
                &messages::Optional::has_f4
            )
        )
    };
}

template <typename T>
void WriteOptionalStruct(ups::io::WriteContext& ctx, T&& obj, messages::Optional& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Optional::kF1FieldNumber,
            &messages::Optional::set_f1,
            &messages::Optional::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Optional::kF2FieldNumber,
            &messages::Optional::set_f2<const std::string&>,
            &messages::Optional::set_f2<std::string>,
            &messages::Optional::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Optional::kF3FieldNumber,
            &messages::Optional::set_f3,
            &messages::Optional::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f4,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Optional::kF4FieldNumber,
            &messages::Optional::mutable_f4,
            &messages::Optional::clear_f4
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Optional& obj, messages::Optional& msg) {
    WriteOptionalStruct(ctx, obj, msg);
}
void WriteProtoStruct(ups::io::WriteContext& ctx, Optional&& obj, messages::Optional& msg) {
    WriteOptionalStruct(ctx, std::move(obj), msg);
}

Repeated ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Repeated>, const messages::Repeated& msg) {
    return {
        .f1 = ups::io::impl::ReadField<std::vector<int32_t>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Repeated::kF1FieldNumber, &messages::Repeated::f1)
        ),
        .f2 = ups::io::impl::ReadField<std::vector<std::string>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Repeated::kF2FieldNumber, &messages::Repeated::f2)
        ),
        .f3 = ups::io::impl::ReadField<std::vector<TestEnum>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Repeated::kF3FieldNumber, &messages::Repeated::f3)
        ),
        .f4 = ups::io::impl::ReadField<std::vector<Simple>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Repeated::kF4FieldNumber, &messages::Repeated::f4)
        )
    };
}

template <typename T>
void WriteRepeatedStruct(ups::io::WriteContext& ctx, T&& obj, messages::Repeated& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Repeated::kF1FieldNumber,
            &messages::Repeated::mutable_f1,
            &messages::Repeated::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Repeated::kF2FieldNumber,
            &messages::Repeated::mutable_f2,
            &messages::Repeated::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Repeated::kF3FieldNumber,
            &messages::Repeated::mutable_f3,
            &messages::Repeated::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f4,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Repeated::kF4FieldNumber,
            &messages::Repeated::mutable_f4,
            &messages::Repeated::clear_f4
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Repeated& obj, messages::Repeated& msg) {
    WriteRepeatedStruct(ctx, obj, msg);
}

void WriteProtoStruct(ups::io::WriteContext& ctx, Repeated&& obj, messages::Repeated& msg) {
    WriteRepeatedStruct(ctx, std::move(obj), msg);
}

Map ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Map>, const messages::Map& msg) {
    return {
        .f1 = ups::io::impl::ReadField<std::map<
            int32_t,
            int32_t>>(ctx, ups::io::impl::CreateFieldGetter(msg, messages::Map::kF1FieldNumber, &messages::Map::f1)),
        .f2 = ups::io::impl::ReadField<std::unordered_map<std::string, std::string>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Map::kF2FieldNumber, &messages::Map::f2)
        ),
        .f3 = ups::io::impl::ReadField<std::map<
            bool,
            TestEnum>>(ctx, ups::io::impl::CreateFieldGetter(msg, messages::Map::kF3FieldNumber, &messages::Map::f3)),
        .f4 = ups::io::impl::ReadField<std::unordered_map<std::string, Simple, USERVER_NAMESPACE::utils::StrCaseHash>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Map::kF4FieldNumber, &messages::Map::f4)
        )
    };
}

template <typename T>
void WriteMapStruct(ups::io::WriteContext& ctx, T&& obj, messages::Map& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Map::kF1FieldNumber,
            &messages::Map::mutable_f1,
            &messages::Map::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Map::kF2FieldNumber,
            &messages::Map::mutable_f2,
            &messages::Map::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Map::kF3FieldNumber,
            &messages::Map::mutable_f3,
            &messages::Map::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f4,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Map::kF4FieldNumber,
            &messages::Map::mutable_f4,
            &messages::Map::clear_f4
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Map& obj, messages::Map& msg) { WriteMapStruct(ctx, obj, msg); }

void WriteProtoStruct(ups::io::WriteContext& ctx, Map&& obj, messages::Map& msg) {
    WriteMapStruct(ctx, std::move(obj), msg);
}

Oneof ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Oneof>, const messages::Oneof& msg) {
    return {
        .test_oneof = ups::io::impl::ReadField<Oneof::Type>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Oneof::kF1FieldNumber,
                &messages::Oneof::f1,
                &messages::Oneof::has_f1
            ),
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Oneof::kF2FieldNumber,
                &messages::Oneof::f2,
                &messages::Oneof::has_f2
            ),
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Oneof::kF3FieldNumber,
                &messages::Oneof::f3,
                &messages::Oneof::has_f3
            ),
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Oneof::kF4FieldNumber,
                &messages::Oneof::f4,
                &messages::Oneof::has_f4
            )
        )
    };
}

template <typename T>
void WriteOneofStruct(ups::io::WriteContext& ctx, T&& obj, messages::Oneof& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).test_oneof,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Oneof::kF1FieldNumber,
            &messages::Oneof::set_f1,
            &messages::Oneof::clear_f1
        ),
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Oneof::kF2FieldNumber,
            &messages::Oneof::set_f2<const std::string&>,
            &messages::Oneof::set_f2<std::string>,
            &messages::Oneof::clear_f2
        ),
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Oneof::kF3FieldNumber,
            &messages::Oneof::set_f3,
            &messages::Oneof::clear_f3
        ),
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Oneof::kF4FieldNumber,
            &messages::Oneof::mutable_f4,
            &messages::Oneof::clear_f4
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Oneof& obj, messages::Oneof& msg) {
    WriteOneofStruct(ctx, obj, msg);
}

void WriteProtoStruct(ups::io::WriteContext& ctx, Oneof&& obj, messages::Oneof& msg) {
    WriteOneofStruct(ctx, std::move(obj), msg);
}

Indirect ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Indirect>, const messages::Indirect& msg) {
    return {
        .f1 = ups::io::impl::ReadField<Indirect::Box<Simple>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Indirect::kF1FieldNumber, &messages::Indirect::f1)
        ),
        .f2 = ups::io::impl::ReadField<std::optional<Indirect::Box<ups::Duration>>>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Indirect::kF2FieldNumber,
                &messages::Indirect::f2,
                &messages::Indirect::has_f2
            )
        ),
        .f3 = ups::io::impl::ReadField<std::vector<Indirect::Box<Simple>>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Indirect::kF3FieldNumber, &messages::Indirect::f3)
        ),
        .f4 = ups::io::impl::ReadField<std::map<int32_t, Indirect::Box<Simple>>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Indirect::kF4FieldNumber, &messages::Indirect::f4)
        ),
        .test_oneof = ups::io::impl::ReadField<Indirect::OneofType>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Indirect::kF5FieldNumber,
                &messages::Indirect::f5,
                &messages::Indirect::has_f5
            ),
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Indirect::kF6FieldNumber,
                &messages::Indirect::f6,
                &messages::Indirect::has_f6
            )
        ),
        .f7 = ups::io::impl::ReadField<Indirect::Box<int32_t>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Indirect::kF7FieldNumber, &messages::Indirect::f7)
        ),
        .f8 = ups::io::impl::ReadField<Indirect::Box<std::vector<Indirect::Box<TestEnum>>>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Indirect::kF8FieldNumber, &messages::Indirect::f8)
        ),
        .f9 = ups::io::impl::ReadField<Indirect::Box<std::map<std::string, Indirect::Box<Simple>>>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Indirect::kF9FieldNumber, &messages::Indirect::f9)
        )
    };
}

template <typename T>
void WriteIndirectStruct(ups::io::WriteContext& ctx, T&& obj, messages::Indirect& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF1FieldNumber,
            &messages::Indirect::mutable_f1,
            &messages::Indirect::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF2FieldNumber,
            &messages::Indirect::mutable_f2,
            &messages::Indirect::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF3FieldNumber,
            &messages::Indirect::mutable_f3,
            &messages::Indirect::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f4,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF4FieldNumber,
            &messages::Indirect::mutable_f4,
            &messages::Indirect::clear_f4
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).test_oneof,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF5FieldNumber,
            &messages::Indirect::mutable_f5,
            &messages::Indirect::clear_f5
        ),
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF6FieldNumber,
            &messages::Indirect::set_f6<const std::string&>,
            &messages::Indirect::set_f6<std::string>,
            &messages::Indirect::clear_f6
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f7,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF7FieldNumber,
            &messages::Indirect::set_f7,
            &messages::Indirect::clear_f7
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f8,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF8FieldNumber,
            &messages::Indirect::mutable_f8,
            &messages::Indirect::clear_f8
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f9,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Indirect::kF9FieldNumber,
            &messages::Indirect::mutable_f9,
            &messages::Indirect::clear_f9
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Indirect& obj, messages::Indirect& msg) {
    WriteIndirectStruct(ctx, obj, msg);
}

void WriteProtoStruct(ups::io::WriteContext& ctx, Indirect&& obj, messages::Indirect& msg) {
    WriteIndirectStruct(ctx, std::move(obj), msg);
}

Strong ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Strong>, const messages::Strong& msg) {
    return {
        .f1 = ups::io::impl::ReadField<USERVER_NAMESPACE::utils::StrongTypedef<Strong::Tag, int32_t>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Strong::kF1FieldNumber, &messages::Strong::f1)
        ),
        .f2 = ups::io::impl::ReadField<
            std::optional<USERVER_NAMESPACE::utils::StrongTypedef<Strong::Tag, std::string>>>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Strong::kF2FieldNumber,
                &messages::Strong::f2,
                &messages::Strong::has_f2
            )
        ),
        .f3 = ups::io::impl::ReadField<std::vector<USERVER_NAMESPACE::utils::StrongTypedef<Strong::Tag, TestEnum>>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Strong::kF3FieldNumber, &messages::Strong::f3)
        ),
        .f4 = ups::io::impl::ReadField<std::map<int32_t, USERVER_NAMESPACE::utils::StrongTypedef<Strong::Tag, Simple>>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Strong::kF4FieldNumber, &messages::Strong::f4)
        ),
        .test_oneof = ups::io::impl::ReadField<Strong::OneofType>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Strong::kF5FieldNumber,
                &messages::Strong::f5,
                &messages::Strong::has_f5
            )
        )
    };
}

template <typename T>
void WriteStrongStruct(ups::io::WriteContext& ctx, T&& obj, messages::Strong& msg) {
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Strong::kF1FieldNumber,
            &messages::Strong::set_f1,
            &messages::Strong::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Strong::kF2FieldNumber,
            &messages::Strong::set_f2<const std::string&>,
            &messages::Strong::set_f2<std::string>,
            &messages::Strong::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Strong::kF3FieldNumber,
            &messages::Strong::mutable_f3,
            &messages::Strong::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).f4,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Strong::kF4FieldNumber,
            &messages::Strong::mutable_f4,
            &messages::Strong::clear_f4
        )
    );
    ups::io::impl::WriteField(
        ctx,
        std::forward<T>(obj).test_oneof,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Strong::kF1FieldNumber,
            &messages::Strong::mutable_f5,
            &messages::Strong::clear_f5
        )
    );
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Strong& obj, messages::Strong& msg) {
    WriteStrongStruct(ctx, obj, msg);
}

void WriteProtoStruct(ups::io::WriteContext& ctx, Strong&& obj, messages::Strong& msg) {
    WriteStrongStruct(ctx, std::move(obj), msg);
}

Erroneous ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Erroneous>, const messages::Erroneous& msg) {
    return {
        .f1 = ups::io::impl::ReadField<std::optional<ConversionFailure>>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Erroneous::kF1FieldNumber,
                &messages::Erroneous::f1,
                &messages::Erroneous::has_f1
            )
        ),
        .f2 = ups::io::impl::ReadField<std::vector<ConversionFailure>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Erroneous::kF2FieldNumber, &messages::Erroneous::f2)
        ),
        .f3 = ups::io::impl::ReadField<std::unordered_map<int32_t, ConversionFailure>>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Erroneous::kF3FieldNumber, &messages::Erroneous::f3)
        ),
        .test_oneof = ups::io::impl::ReadField<Erroneous::OneofType>(
            ctx,
            ups::io::impl::CreateFieldGetter(
                msg,
                messages::Erroneous::kF4FieldNumber,
                &messages::Erroneous::f4,
                &messages::Erroneous::has_f4
            )
        )
    };
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Erroneous& obj, messages::Erroneous& msg) {
    ups::io::impl::WriteField(
        ctx,
        obj.f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Erroneous::kF1FieldNumber,
            &messages::Erroneous::mutable_f1,
            &messages::Erroneous::clear_f1
        )
    );
    ups::io::impl::WriteField(
        ctx,
        obj.f2,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Erroneous::kF2FieldNumber,
            &messages::Erroneous::mutable_f2,
            &messages::Erroneous::clear_f2
        )
    );
    ups::io::impl::WriteField(
        ctx,
        obj.f3,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Erroneous::kF3FieldNumber,
            &messages::Erroneous::mutable_f3,
            &messages::Erroneous::clear_f3
        )
    );
    ups::io::impl::WriteField(
        ctx,
        obj.test_oneof,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Erroneous::kF4FieldNumber,
            &messages::Erroneous::mutable_f4,
            &messages::Erroneous::clear_f4
        )
    );
}

void CheckScalarEqual(const Scalar& obj, const messages::Scalar& msg) {
    EXPECT_EQ(obj.f1, msg.f1());
    EXPECT_EQ(obj.f2, msg.f2());
    EXPECT_EQ(obj.f3, msg.f3());
    EXPECT_EQ(obj.f4, msg.f4());
    EXPECT_EQ(obj.f5, msg.f5());
    EXPECT_EQ(obj.f6, msg.f6());
    EXPECT_EQ(obj.f7, msg.f7());
    EXPECT_EQ(obj.f8, msg.f8());
    EXPECT_EQ(obj.f9, msg.f9());
    EXPECT_EQ(obj.f10, static_cast<TestEnum>(msg.f10()));
    EXPECT_EQ(static_cast<int>(obj.f11), msg.f11());
}

void CheckWellKnownStdEqual(const WellKnownStd& obj, const messages::WellKnownStd& msg) {
    using TimeType = std::chrono::time_point<std::chrono::system_clock>;

    EXPECT_EQ(
        obj.f1.time_since_epoch(),
        std::chrono::duration_cast<TimeType::duration>(std::chrono::nanoseconds{
            ::google::protobuf::util::TimeUtil::TimestampToNanoseconds(msg.f1())
        })
    );
    EXPECT_EQ(
        obj.f2,
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds{
            ::google::protobuf::util::TimeUtil::DurationToNanoseconds(msg.f2())
        })
    );

    EXPECT_EQ(static_cast<int>(obj.f3.year()), msg.f3().year());
    EXPECT_EQ(static_cast<unsigned>(obj.f3.month()), static_cast<unsigned>(msg.f3().month()));
    EXPECT_EQ(static_cast<unsigned>(obj.f3.day()), static_cast<unsigned>(msg.f3().day()));

    EXPECT_EQ(obj.f4.hours().count(), msg.f4().hours());
    EXPECT_EQ(obj.f4.minutes().count(), msg.f4().minutes());
    EXPECT_EQ(obj.f4.seconds().count(), msg.f4().seconds());
    EXPECT_EQ(
        obj.f4.subseconds().count(),
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(msg.f4().nanos())).count()
    );
}

void CheckWellKnownUsrvEqual(const WellKnownUsrv& obj, const messages::WellKnownUsrv& msg) {
    EXPECT_EQ(obj.f1.GetProtobufAny().type_url(), msg.f1().type_url());
    EXPECT_EQ(obj.f1.GetProtobufAny().value(), msg.f1().value());

    EXPECT_EQ(obj.f2.Seconds().count(), msg.f2().seconds());
    EXPECT_EQ(obj.f2.Nanos().count(), msg.f2().nanos());

    EXPECT_EQ(obj.f3.Seconds().count(), msg.f3().seconds());
    EXPECT_EQ(obj.f3.Nanos().count(), msg.f3().nanos());

    EXPECT_EQ(obj.f4.YearNum(), msg.f4().year());
    EXPECT_EQ(obj.f4.MonthNum(), msg.f4().month());
    EXPECT_EQ(obj.f4.DayNum(), msg.f4().day());

    EXPECT_EQ(obj.f5.Hours().count(), msg.f5().hours());
    EXPECT_EQ(obj.f5.Minutes().count(), msg.f5().minutes());
    EXPECT_EQ(obj.f5.Seconds().count(), msg.f5().seconds());
    EXPECT_EQ(obj.f5.Nanos().count(), msg.f5().nanos());

    EXPECT_EQ(obj.f6.Hours().count(), msg.f6().hours());
    EXPECT_EQ(obj.f6.Minutes().count(), msg.f6().minutes());
    EXPECT_EQ(obj.f6.Seconds().count(), msg.f6().seconds());
    EXPECT_EQ(
        obj.f6.Subseconds().count(),
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(msg.f6().nanos())).count()
    );

    EXPECT_EQ(obj.f7, decimal64::Decimal<3>(msg.f7().value()));
    EXPECT_EQ(ToString(obj.f7), msg.f7().value());
}

void CheckWellKnownJsonEqual(const WellKnownJson& obj, const messages::WellKnownJson& msg) {
    CheckJsonValue(obj.f1, msg.f1());
    CheckJsonValue(obj.f2.GetValue(), msg.f2());
    CheckJsonValue(obj.f3.GetValue(), msg.f3());
}

void CheckOptionalEqual(const Optional& obj, const messages::Optional& msg) {
    if (obj.f1) {
        EXPECT_TRUE(msg.has_f1());
        EXPECT_EQ(obj.f1.value(), msg.f1());
    } else {
        EXPECT_FALSE(msg.has_f1());
    }

    if (obj.f2) {
        EXPECT_TRUE(msg.has_f2());
        EXPECT_EQ(obj.f2.value(), msg.f2());
    } else {
        EXPECT_FALSE(msg.has_f2());
    }

    if (obj.f3) {
        EXPECT_TRUE(msg.has_f3());
        EXPECT_EQ(obj.f3.value(), static_cast<TestEnum>(msg.f3()));
    } else {
        EXPECT_FALSE(msg.has_f3());
    }

    if (obj.f4) {
        EXPECT_TRUE(msg.has_f4());
        CheckSimpleEqual(obj.f4.value(), msg.f4());
    } else {
        EXPECT_FALSE(msg.has_f4());
    }
}

void CheckRepeatedEqual(const Repeated& obj, const messages::Repeated& msg) {
    ASSERT_EQ(obj.f1.size(), static_cast<std::size_t>(msg.f1().size()));

    for (int i = 0; i < msg.f1().size(); ++i) {
        EXPECT_EQ(obj.f1[i], msg.f1()[i]);
    }

    ASSERT_EQ(obj.f2.size(), static_cast<std::size_t>(msg.f2().size()));

    for (int i = 0; i < msg.f2().size(); ++i) {
        EXPECT_EQ(obj.f2[i], msg.f2()[i]);
    }

    ASSERT_EQ(obj.f3.size(), static_cast<std::size_t>(msg.f3().size()));

    for (int i = 0; i < msg.f3().size(); ++i) {
        EXPECT_EQ(static_cast<int>(obj.f3[i]), msg.f3()[i]);
    }

    ASSERT_EQ(obj.f4.size(), static_cast<std::size_t>(msg.f4().size()));

    for (int i = 0; i < msg.f4().size(); ++i) {
        EXPECT_EQ(obj.f4[i].f1, msg.f4()[i].f1());
    }
}

void CheckMapEqual(const Map& obj, const messages::Map& msg) {
    ASSERT_EQ(obj.f1.size(), static_cast<std::size_t>(msg.f1().size()));

    for (const auto& [key, value] : msg.f1()) {
        ASSERT_EQ(obj.f1.count(key), std::size_t{1});
        EXPECT_EQ(obj.f1.at(key), value);
    }

    ASSERT_EQ(obj.f2.size(), static_cast<std::size_t>(msg.f2().size()));

    for (const auto& [key, value] : msg.f2()) {
        ASSERT_EQ(obj.f2.count(key), std::size_t{1});
        EXPECT_EQ(obj.f2.at(key), value);
    }

    ASSERT_EQ(obj.f3.size(), static_cast<std::size_t>(msg.f3().size()));

    for (const auto& [key, value] : msg.f3()) {
        ASSERT_EQ(obj.f3.count(key), std::size_t{1});
        EXPECT_EQ(obj.f3.at(key), static_cast<TestEnum>(value));
    }

    ASSERT_EQ(obj.f4.size(), static_cast<std::size_t>(msg.f4().size()));

    for (const auto& [key, value] : msg.f4()) {
        ASSERT_EQ(obj.f4.count(key), std::size_t{1});
        CheckSimpleEqual(obj.f4.at(key), value);
    }
}

void CheckOneofEqual(const Oneof& obj, const messages::Oneof& msg) {
    if (!obj.test_oneof.ContainsAny()) {
        EXPECT_EQ(msg.test_oneof_case(), messages::Oneof::TEST_ONEOF_NOT_SET);
    }

    if (obj.test_oneof.Contains(0)) {
        EXPECT_TRUE(msg.has_f1());
        EXPECT_EQ(obj.test_oneof.Get<0>(), msg.f1());
    } else {
        EXPECT_FALSE(msg.has_f1());
    }

    if (obj.test_oneof.Contains(1)) {
        EXPECT_TRUE(msg.has_f2());
        EXPECT_EQ(obj.test_oneof.Get<1>(), msg.f2());
    } else {
        EXPECT_FALSE(msg.has_f2());
    }

    if (obj.test_oneof.Contains(2)) {
        EXPECT_TRUE(msg.has_f3());
        EXPECT_EQ(obj.test_oneof.Get<2>(), static_cast<TestEnum>(msg.f3()));
    } else {
        EXPECT_FALSE(msg.has_f3());
    }

    if (obj.test_oneof.Contains(3)) {
        EXPECT_TRUE(msg.has_f4());
        CheckSimpleEqual(obj.test_oneof.Get<3>(), msg.f4());
    } else {
        EXPECT_FALSE(msg.has_f4());
    }
}

void CheckIndirectEqual(const Indirect& obj, const messages::Indirect& msg) {
    CheckSimpleEqual(*obj.f1, msg.f1());

    if (obj.f2) {
        EXPECT_TRUE(msg.has_f2());
        EXPECT_EQ((*obj.f2.value()).Seconds().count(), msg.f2().seconds());
        EXPECT_EQ((*obj.f2.value()).Nanos().count(), msg.f2().nanos());
    } else {
        EXPECT_FALSE(msg.has_f2());
    }

    ASSERT_EQ(obj.f3.size(), static_cast<std::size_t>(msg.f3().size()));

    for (int i = 0; i < msg.f3().size(); ++i) {
        CheckSimpleEqual(*obj.f3[i], msg.f3()[i]);
    }

    ASSERT_EQ(obj.f4.size(), static_cast<std::size_t>(msg.f4().size()));

    for (const auto& [key, value] : msg.f4()) {
        ASSERT_EQ(obj.f4.count(key), std::size_t{1});
        CheckSimpleEqual(*obj.f4.at(key), value);
    }

    if (!obj.test_oneof.ContainsAny()) {
        EXPECT_EQ(msg.test_oneof_case(), messages::Indirect::TEST_ONEOF_NOT_SET);
    }

    if (obj.test_oneof.Contains(0)) {
        EXPECT_TRUE(msg.has_f5());
        CheckSimpleEqual(*obj.test_oneof.Get<0>(), msg.f5());
    } else {
        EXPECT_FALSE(msg.has_f5());
    }

    if (obj.test_oneof.Contains(1)) {
        EXPECT_TRUE(msg.has_f6());
        EXPECT_EQ(*obj.test_oneof.Get<1>(), msg.f6());
    } else {
        EXPECT_FALSE(msg.has_f6());
    }

    EXPECT_EQ(*obj.f7, msg.f7());
    ASSERT_EQ((*obj.f8).size(), static_cast<std::size_t>(msg.f8().size()));

    for (int i = 0; i < msg.f8().size(); ++i) {
        EXPECT_EQ(*((*obj.f8)[i]), static_cast<TestEnum>(msg.f8()[i]));
    }

    ASSERT_EQ((*obj.f9).size(), static_cast<std::size_t>(msg.f9().size()));

    for (const auto& [key, value] : msg.f9()) {
        ASSERT_EQ((*obj.f9).count(key), std::size_t{1});
        CheckSimpleEqual(*((*obj.f9).at(key)), value);
    }
}

void CheckStrongEqual(const Strong& obj, const messages::Strong& msg) {
    EXPECT_EQ(obj.f1.GetUnderlying(), msg.f1());

    if (obj.f2) {
        EXPECT_TRUE(msg.has_f2());
        EXPECT_EQ(obj.f2.value().GetUnderlying(), msg.f2());
    } else {
        EXPECT_FALSE(msg.has_f2());
    }

    ASSERT_EQ(obj.f3.size(), static_cast<std::size_t>(msg.f3().size()));

    for (int i = 0; i < msg.f3().size(); ++i) {
        EXPECT_EQ(static_cast<int>(obj.f3[i].GetUnderlying()), msg.f3()[i]);
    }

    ASSERT_EQ(obj.f4.size(), static_cast<std::size_t>(msg.f4().size()));

    for (const auto& [key, value] : msg.f4()) {
        ASSERT_EQ(obj.f4.count(key), std::size_t{1});
        CheckSimpleEqual(obj.f4.at(key).GetUnderlying(), value);
    }

    if (!obj.test_oneof.ContainsAny()) {
        EXPECT_EQ(msg.test_oneof_case(), messages::Strong::TEST_ONEOF_NOT_SET);
    }

    if (obj.test_oneof.Contains(0)) {
        EXPECT_TRUE(msg.has_f5());
        EXPECT_EQ(obj.test_oneof.Get<0>().GetUnderlying().Seconds().count(), msg.f5().seconds());
        EXPECT_EQ(obj.test_oneof.Get<0>().GetUnderlying().Nanos().count(), msg.f5().nanos());

    } else {
        EXPECT_FALSE(msg.has_f5());
    }
}

}  // namespace structs
