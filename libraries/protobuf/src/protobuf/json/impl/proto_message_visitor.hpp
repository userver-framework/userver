#pragma once

/// @file
/// @brief Reusable ProtoJSON traversal of a protobuf message.
///
/// @ref ProtoMessageVisitor walks a protobuf message according to the
/// [ProtoJSON](https://protobuf.dev/programming-guides/json/) rules (field presence, `json_name`, repeated/map,
/// well-known types, value extraction via reflection) and feeds the result into a user-provided @a Handler through
/// typed callbacks. The handler is responsible for the concrete output (a `formats::json::ValueBuilder` DOM, a JSON
/// string via `formats::json::StringBuilder`, etc.) and for the ProtoJSON representation of individual values.
///
/// Handler concept (see the handlers in write.cpp / string_writer.hpp for real implementations):
/// @code
/// class Handler {
/// public:
///     // RAII scopes for containers: ctor opens, dtor closes (LIFO order is guaranteed by the visitor).
///     class ObjectGuard { public: explicit ObjectGuard(Handler&); ~ObjectGuard(); };
///     class ArrayGuard  { public: explicit ArrayGuard(Handler&);  ~ArrayGuard();  };
///
///     void Key(std::string_view key);            // object member name / map key
///
///     void Null();
///     void Bool(bool);
///     void Int32(std::int32_t);                  // ProtoJSON: number
///     void UInt32(std::uint32_t);                // number
///     void Int64(std::int64_t);                  // ProtoJSON: string (handler does to_string)
///     void UInt64(std::uint64_t);                // string
///     void Float(float);                         // nan/inf -> string, otherwise number
///     void Double(double);                       // nan/inf -> string, otherwise number
///     void String(std::string_view);
///     void Bytes(std::string_view raw);          // handler base64-encodes
///
///     [[nodiscard]] bool LimitReached() const;   // true -> stop the traversal (size limit); otherwise always false
/// };
/// @endcode

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/reflection.h>

#include <protobuf/json/impl/convert_utils.hpp>
#include <protobuf/json/impl/field_error.hpp>
#include <userver/protobuf/datetime.hpp>
#include <userver/protobuf/json/convert_options.hpp>
#include <userver/protobuf/json/exceptions.hpp>
#include <userver/protobuf/string.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

[[nodiscard]] inline bool HasDebugRedactOption([[maybe_unused]] const google::protobuf::FieldDescriptor& field) {
#if defined(ARCADIA_ROOT) || GOOGLE_PROTOBUF_VERSION >= 4022000
    return field.options().debug_redact();
#else
    return false;
#endif
}

[[nodiscard]] inline bool HasField(
    const ::google::protobuf::Message& message,
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::FieldDescriptor& field
) {
    if (!field.is_repeated()) {
        return reflection.HasField(message, &field);
    } else {
        return 0 != reflection.FieldSize(message, &field);
    }
}

[[nodiscard]] inline bool IsClearedMessage(const ::google::protobuf::Message& message) {
    const auto& desc = *message.GetDescriptor();
    const auto& reflection = *message.GetReflection();

    for (int i = 0; i < desc.field_count(); ++i) {
        if (HasField(message, reflection, *desc.field(i))) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] inline std::string MapKeyToString(
    const ::google::protobuf::Reflection& reflection,
    const ::google::protobuf::Message& entry,
    const ::google::protobuf::FieldDescriptor& key_desc
) {
    using ::google::protobuf::FieldDescriptor;

    switch (key_desc.type()) {
        case FieldDescriptor::TYPE_INT64:
        case FieldDescriptor::TYPE_SFIXED64:
        case FieldDescriptor::TYPE_SINT64:
            return std::to_string(reflection.GetInt64(entry, &key_desc));

        case FieldDescriptor::TYPE_UINT64:
        case FieldDescriptor::TYPE_FIXED64:
            return std::to_string(reflection.GetUInt64(entry, &key_desc));

        case FieldDescriptor::TYPE_INT32:
        case FieldDescriptor::TYPE_SFIXED32:
        case FieldDescriptor::TYPE_SINT32:
            return std::to_string(reflection.GetInt32(entry, &key_desc));

        case FieldDescriptor::TYPE_UINT32:
        case FieldDescriptor::TYPE_FIXED32:
            return std::to_string(reflection.GetUInt32(entry, &key_desc));

        case FieldDescriptor::TYPE_BOOL:
            return reflection.GetBool(entry, &key_desc) ? "true" : "false";

        case FieldDescriptor::TYPE_STRING:
            return reflection.GetString(entry, &key_desc);

        default:
            UINVARIANT(false, "Unexpected protobuf map key descriptor type");
    }
}

// Reads a singular (non-repeated) protobuf field via reflection (the trailing index argument is ignored).
class SingularFieldGetter {
public:
    explicit SingularFieldGetter(const ::google::protobuf::Reflection& reflection)
        : reflection_{reflection}
    {}

    [[nodiscard]] std::int64_t GetInt64(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetInt64(message, &field);
    }

    [[nodiscard]] std::uint64_t GetUInt64(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetUInt64(message, &field);
    }

    [[nodiscard]] std::int32_t GetInt32(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetInt32(message, &field);
    }

    [[nodiscard]] std::uint32_t GetUInt32(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetUInt32(message, &field);
    }

    [[nodiscard]] bool GetBool(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetBool(message, &field);
    }

    [[nodiscard]] float GetFloat(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetFloat(message, &field);
    }

    [[nodiscard]] double GetDouble(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetDouble(message, &field);
    }

    [[nodiscard]] const ProtoStringType& GetString(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field,
        ProtoStringType& scratch
    ) const {
        return reflection_.GetStringReference(message, &field, &scratch);
    }

    [[nodiscard]] const ::google::protobuf::EnumValueDescriptor& GetEnum(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return *reflection_.GetEnum(message, &field);
    }

    [[nodiscard]] const ::google::protobuf::Message& GetMessage(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetMessage(message, &field);
    }

private:
    const ::google::protobuf::Reflection& reflection_;
};

// Reads an element of a repeated protobuf field via reflection.
class RepeatedFieldGetter {
public:
    RepeatedFieldGetter(const ::google::protobuf::Reflection& reflection, int index)
        : reflection_{reflection},
          index_{index}
    {}

    [[nodiscard]] std::int64_t GetInt64(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetRepeatedInt64(message, &field, index_);
    }

    [[nodiscard]] std::uint64_t GetUInt64(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetRepeatedUInt64(message, &field, index_);
    }

    [[nodiscard]] std::int32_t GetInt32(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetRepeatedInt32(message, &field, index_);
    }

    [[nodiscard]] std::uint32_t GetUInt32(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetRepeatedUInt32(message, &field, index_);
    }

    [[nodiscard]] bool GetBool(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetRepeatedBool(message, &field, index_);
    }

    [[nodiscard]] float GetFloat(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetRepeatedFloat(message, &field, index_);
    }

    [[nodiscard]] double GetDouble(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetRepeatedDouble(message, &field, index_);
    }

    [[nodiscard]] const ProtoStringType& GetString(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field,
        ProtoStringType& scratch
    ) const {
        return reflection_.GetRepeatedStringReference(message, &field, index_, &scratch);
    }

    [[nodiscard]] const ::google::protobuf::EnumValueDescriptor& GetEnum(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return *reflection_.GetRepeatedEnum(message, &field, index_);
    }

    [[nodiscard]] const ::google::protobuf::Message& GetMessage(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field
    ) const {
        return reflection_.GetRepeatedMessage(message, &field, index_);
    }

private:
    const ::google::protobuf::Reflection& reflection_;
    const int index_{};
};

/// @brief Traverses a protobuf message following the ProtoJSON rules and drives a typed @a Handler.
///
/// The visitor itself is not a template; @a Handler is a template parameter of @ref Visit (and of the private
/// traversal helpers), so a single visitor object can drive different handlers.
template <typename Handler>
class ProtoMessageVisitor final {
public:
    /// Print options are configured after construction via the `Set*`/`Get*` accessors below:
    /// * `always_print_fields_with_no_presence` — emit implicit-presence fields left at default (default false);
    /// * `always_print_enums_as_ints` — print enum values as integers instead of their string names (default false);
    /// * `preserve_proto_field_names` — use proto field names instead of `json_name` (default false);
    /// * `expand_any` — parse and expand `google.protobuf.Any` into `{"@type": ..., ...}`; when false `Any` is emitted
    ///   raw as `{"typeUrl"|"type_url": ..., "value": "<base64>"}` (default false, explicit opt-in);
    /// * `expand_any_fallback_to_raw` — when `Any` expansion fails (payload type not found in the descriptor pool, or
    ///   payload failed to parse), fall back to the same raw representation instead of throwing `FieldError`; only
    ///   takes effect when `expand_any` is true (default false);
    /// * `redact_debug_string` — replace `[debug_redact = true]` field values with a `"[REDACTED]"` marker
    ///   (default false).
    explicit ProtoMessageVisitor(Handler& handler)
        : handler_{handler}
    {}

    void SetAlwaysPrintFieldsWithNoPresence(bool value) { always_print_fields_with_no_presence_ = value; }

    void SetAlwaysPrintEnumsAsInts(bool value) { always_print_enums_as_ints_ = value; }

    void SetPreserveProtoFieldNames(bool value) { preserve_proto_field_names_ = value; }

    void SetExpandAny(bool value) { expand_any_ = value; }

    void SetExpandAnyFallbackToRaw(bool value) { expand_any_fallback_to_raw_ = value; }

    void SetRedactDebugString(bool value) { redact_debug_string_ = value; }

    /// @brief Traverse @a message, feeding @a handler.
    /// @throws FieldError on conversion failure; the caller is responsible for wrapping it into a PrintError.
    void operator()(const ::google::protobuf::Message& message) { VisitMessage(message); }

private:
    void VisitMessage(const ::google::protobuf::Message& message) {
        VisitMessage(ClassifyMessage(message.GetDescriptor()->full_name()), message);
    }

    void VisitMessage(MessageType type, const ::google::protobuf::Message& message) {
        switch (type) {
            case MessageType::kGeneral:
                VisitGeneral(message);
                return;
            case MessageType::kAny:
                VisitAny(message);
                return;
            case MessageType::kDuration:
                VisitDuration(message);
                return;
            case MessageType::kTimestamp:
                VisitTimestamp(message);
                return;
            case MessageType::kFieldMask:
                VisitFieldMask(message);
                return;
            case MessageType::kValue:
                VisitValue(message);
                return;
            case MessageType::kListValue:
                VisitListValue(message);
                return;
            case MessageType::kStruct:
                VisitStruct(message);
                return;
            case MessageType::kDoubleValue:
                VisitDoubleValue(message);
                return;
            case MessageType::kFloatValue:
                VisitFloatValue(message);
                return;
            case MessageType::kInt64Value:
                VisitInt64Value(message);
                return;
            case MessageType::kUInt64Value:
                VisitUInt64Value(message);
                return;
            case MessageType::kInt32Value:
                VisitInt32Value(message);
                return;
            case MessageType::kUInt32Value:
                VisitUInt32Value(message);
                return;
            case MessageType::kBoolValue:
                VisitBoolValue(message);
                return;
            case MessageType::kStringValue:
                VisitStringValue(message);
                return;
            case MessageType::kBytesValue:
                VisitBytesValue(message);
                return;
        }
    }

    void VisitFieldValue(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc
    ) {
        try {
            SingularFieldGetter singular_field_getter{reflection};
            VisitFieldValue(singular_field_getter, message, field_desc);
        } catch (FieldError& error) {
            error.PrependField(field_desc.name());
            throw;
        }
    }

    void VisitRepeatedFieldValue(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc,
        int index
    ) {
        RepeatedFieldGetter repeated_field_getter{reflection, index};
        VisitFieldValue(repeated_field_getter, message, field_desc);
    }

    template <typename FieldGetter>
    void VisitFieldValue(
        const FieldGetter& field_getter,
        const ::google::protobuf::Message& message,
        const ::google::protobuf::FieldDescriptor& field_desc
    ) {
        using ::google::protobuf::FieldDescriptor;

        UASSERT(field_desc.containing_type() == message.GetDescriptor());

        switch (field_desc.type()) {
            case FieldDescriptor::TYPE_INT64:
            case FieldDescriptor::TYPE_SFIXED64:
            case FieldDescriptor::TYPE_SINT64:
                handler_.Int64(field_getter.GetInt64(message, field_desc));
                return;

            case FieldDescriptor::TYPE_UINT64:
            case FieldDescriptor::TYPE_FIXED64:
                handler_.UInt64(field_getter.GetUInt64(message, field_desc));
                return;

            case FieldDescriptor::TYPE_INT32:
            case FieldDescriptor::TYPE_SFIXED32:
            case FieldDescriptor::TYPE_SINT32:
                handler_.Int32(field_getter.GetInt32(message, field_desc));
                return;

            case FieldDescriptor::TYPE_UINT32:
            case FieldDescriptor::TYPE_FIXED32:
                handler_.UInt32(field_getter.GetUInt32(message, field_desc));
                return;

            case FieldDescriptor::TYPE_BOOL:
                handler_.Bool(field_getter.GetBool(message, field_desc));
                return;

            case FieldDescriptor::TYPE_FLOAT:
                handler_.Float(field_getter.GetFloat(message, field_desc));
                return;

            case FieldDescriptor::TYPE_DOUBLE:
                handler_.Double(field_getter.GetDouble(message, field_desc));
                return;

            case FieldDescriptor::TYPE_STRING: {
                ProtoStringType scratch;
                handler_.String(std::string_view(field_getter.GetString(message, field_desc, scratch)));
                return;
            }

            case FieldDescriptor::TYPE_BYTES: {
                ProtoStringType scratch;
                handler_.Bytes(std::string_view(field_getter.GetString(message, field_desc, scratch)));
                return;
            }

            case FieldDescriptor::TYPE_ENUM: {
                const auto& enum_value_desc = field_getter.GetEnum(message, field_desc);
                if (!IsNullValue(*enum_value_desc.type())) {
                    if (always_print_enums_as_ints_) {
                        handler_.Int32(enum_value_desc.number());
                    } else {
                        handler_.String(std::string_view{enum_value_desc.name()});
                    }
                } else {
                    // 'google.protobuf.NullValue' enum represents null in JSON (also on its own, not only in Value).
                    handler_.Null();
                }
                return;
            }

            case FieldDescriptor::TYPE_GROUP:
            case FieldDescriptor::TYPE_MESSAGE:
                VisitMessage(field_getter.GetMessage(message, field_desc));
                return;
        }
    }

    void VisitRepeated(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc
    ) {
        using ::google::protobuf::FieldDescriptor;

        const typename Handler::ArrayGuard guard{handler_};
        const auto size = reflection.FieldSize(message, &field_desc);
        const bool is_value_field =
            field_desc.type() == FieldDescriptor::TYPE_MESSAGE &&
            ClassifyMessage(field_desc.message_type()->full_name()) == MessageType::kValue;

        for (int i = 0; i < size; ++i) {
            if (handler_.LimitReached()) {
                break;
            }
            if (is_value_field && IsClearedMessage(reflection.GetRepeatedMessage(message, &field_desc, i))) {
                // skipping google.protobuf.Value which has no alternatives set (native ProtoJSON does not treat it as
                // an error during conversion)
                continue;
            }
            try {
                VisitRepeatedFieldValue(message, reflection, field_desc, i);
            } catch (FieldError& error) {
                error.PrependRepeatedItem(field_desc.name(), i);
                throw;
            }
        }
    }

    void VisitMap(
        const ::google::protobuf::Message& message,
        const ::google::protobuf::Reflection& reflection,
        const ::google::protobuf::FieldDescriptor& field_desc
    ) {
        using ::google::protobuf::FieldDescriptor;

        UASSERT(field_desc.is_map());
        UASSERT(field_desc.message_type() != nullptr);

        const typename Handler::ObjectGuard guard{handler_};

        const FieldDescriptor& key_desc = *field_desc.message_type()->map_key();
        const FieldDescriptor& value_desc = *field_desc.message_type()->map_value();
        const auto size = reflection.FieldSize(message, &field_desc);
        const bool is_value_field =
            value_desc.type() == FieldDescriptor::TYPE_MESSAGE &&
            ClassifyMessage(value_desc.message_type()->full_name()) == MessageType::kValue;

        for (int i = 0; i < size; ++i) {
            if (handler_.LimitReached()) {
                break;
            }
            const auto& entry = reflection.GetRepeatedMessage(message, &field_desc, i);
            const auto& entry_reflection = *entry.GetReflection();
            if (is_value_field && IsClearedMessage(entry_reflection.GetMessage(entry, &value_desc))) {
                continue;
            }

            std::string key = MapKeyToString(entry_reflection, entry, key_desc);
            handler_.Key(key);
            try {
                VisitFieldValue(entry, entry_reflection, value_desc);
            } catch (FieldError& error) {
                error.PrependMapItem(field_desc.name(), key);
                throw;
            }
        }
    }

    void VisitGeneralFields(const ::google::protobuf::Message& message) {
        using ::google::protobuf::FieldDescriptor;

        const auto& desc = *message.GetDescriptor();
        const auto& reflection = *message.GetReflection();

        for (int i = 0; i < desc.field_count(); ++i) {
            if (handler_.LimitReached()) {
                break;
            }

            const auto& field_desc = *desc.field(i);
            const bool has_field = HasField(message, reflection, field_desc);
            // Print set fields, and optionally implicit-presence fields left at their default value (ProtoJSON's
            // 'always_print_fields_with_no_presence'). Message fields have presence, so they are not forced to '{}'.
            if (!has_field && !(always_print_fields_with_no_presence_ && !field_desc.has_presence())) {
                continue;
            }

            // Key name: proto field name vs 'json_name' (FieldMask paths keep their own camelCase logic elsewhere).
            const std::string_view
                field_name = preserve_proto_field_names_ ? field_desc.name() : field_desc.json_name();

            if (redact_debug_string_ && impl::HasDebugRedactOption(field_desc)) {
                // Hide the value of fields explicitly marked as sensitive, regardless of their kind.
                handler_.Key(field_name);
                handler_.String("[REDACTED]");
                continue;
            }

            if (!field_desc.is_repeated()) {
                if (field_desc.type() == FieldDescriptor::TYPE_MESSAGE ||
                    field_desc.type() == FieldDescriptor::TYPE_GROUP)
                {
                    if (ClassifyMessage(field_desc.message_type()->full_name()) == MessageType::kValue &&
                        IsClearedMessage(reflection.GetMessage(message, &field_desc)))
                    {
                        // skipping google.protobuf.Value which has no alternatives set
                        continue;
                    }
                }
                handler_.Key(field_name);
                VisitFieldValue(message, reflection, field_desc);
            } else {
                if (!field_desc.is_map()) {
                    handler_.Key(field_name);
                    VisitRepeated(message, reflection, field_desc);
                } else {
                    handler_.Key(field_name);
                    VisitMap(message, reflection, field_desc);
                }
            }
        }
    }

    void VisitGeneral(const ::google::protobuf::Message& message) {
        const typename Handler::ObjectGuard guard{handler_};
        VisitGeneralFields(message);
    }

    void VisitAny(const ::google::protobuf::Message& message) {
        const auto& desc = *message.GetDescriptor();
        const auto&
            type_url_desc = GetMessageFieldDesc(desc, AnyTraits::kTypeUrlFieldNumber, AnyTraits::kTypeUrlFieldType);
        const auto& value_desc = GetMessageFieldDesc(desc, AnyTraits::kValueFieldNumber, AnyTraits::kValueFieldType);
        const auto& reflection = *message.GetReflection();

        ProtoStringType scratch1;
        ProtoStringType scratch2;
        const auto& type_url = reflection.GetStringReference(message, &type_url_desc, &scratch1);
        const auto& value = reflection.GetStringReference(message, &value_desc, &scratch2);

        const typename Handler::ObjectGuard guard{handler_};

        if (!expand_any_) {
            VisitAnyRaw(std::string_view{type_url}, std::string_view{value});
            return;
        }

        if (type_url.empty() && value.empty()) {
            return;
        }

        const auto payload_desc = FindMessageDescByTypeUrl(*message.GetDescriptor()->file()->pool(), type_url);
        if (!payload_desc) {
            if (expand_any_fallback_to_raw_) {
                VisitAnyRaw(std::string_view{type_url}, std::string_view{value});
                return;
            }
            throw FieldError(PrintErrorCode::kInvalidValue, "can't find 'google.protobuf.Any' payload descriptor");
        }

        ::google::protobuf::DynamicMessageFactory factory;
        {
            // 'payload_message' should be destroyed before 'factory' in case of exception
            std::unique_ptr<::google::protobuf::Message> payload_message(factory.GetPrototype(payload_desc)->New());

            if (!payload_message->ParsePartialFromString(value)) {
                if (expand_any_fallback_to_raw_) {
                    VisitAnyRaw(std::string_view{type_url}, std::string_view{value});
                    return;
                }
                throw FieldError(PrintErrorCode::kInvalidValue, "failed to parse 'google.protobuf.Any' payload");
            }

            // Native ProtoJSON (MessageToJsonString -> WriteAny) always emits "@type" first, before the expanded
            // payload (its fields for a general message, or the "value" member for a well-known type). Keep the same
            // order so the SAX output byte-for-byte matches the reference; this also matches the "@type"-first order
            // documented above and used in the golden test data. The DOM handler is order-insensitive, so this is safe
            // for both handlers.
            handler_.Key("@type");
            handler_.String(std::string_view{type_url});

            const MessageType payload_type = ClassifyMessage(payload_desc->full_name());
            if (payload_type == MessageType::kGeneral) {
                VisitGeneralFields(*payload_message);
            } else {
                handler_.Key("value");
                VisitMessage(payload_type, *payload_message);
            }
        }
    }

    // Raw representation without payload descriptor lookup: {"typeUrl"|"type_url": ..., "value": "<base64>"}.
    // Both fields are emitted unconditionally (even when empty), matching the original WriteAnyMessage.
    void VisitAnyRaw(std::string_view type_url, std::string_view value) {
        handler_.Key(preserve_proto_field_names_ ? "type_url" : "typeUrl");
        handler_.String(type_url);
        handler_.Key("value");
        handler_.Bytes(value);  // Handler base64-encodes the raw bytes.
    }

    void VisitDuration(const ::google::protobuf::Message& message) {
        const auto& desc = *message.GetDescriptor();
        const auto& seconds_desc =
            GetMessageFieldDesc(desc, DurationTraits::kSecondsFieldNumber, DurationTraits::kSecondsFieldType);
        const auto&
            nanos_desc = GetMessageFieldDesc(desc, DurationTraits::kNanosFieldNumber, DurationTraits::kNanosFieldType);
        const auto& reflection = *message.GetReflection();

        const auto seconds = reflection.GetInt64(message, &seconds_desc);
        const auto nanos = reflection.GetInt32(message, &nanos_desc);

        if (!IsValidDuration(seconds, nanos)) {
            throw FieldError(
                PrintErrorCode::kInvalidValue,
                "duration's seconds/nanos combination is invalid or represents out of bounds value"
            );
        }

        std::string value;
        if (nanos == 0) {
            value = fmt::format("{}s", seconds);
        } else {
            std::int32_t seconds_fraction = std::abs(nanos);
            int digits = 9;
            while (seconds_fraction % 1000 == 0) {
                seconds_fraction /= 1000;
                digits -= 3;
            }
            const std::string_view sign = (seconds < 0 || nanos < 0) ? "-" : "";
            value = fmt::format("{}{}.{:0{}}s", sign, std::abs(seconds), seconds_fraction, digits);
        }

        handler_.String(value);
    }

    void VisitTimestamp(const ::google::protobuf::Message& message) {
        const auto& desc = *message.GetDescriptor();
        const auto& seconds_desc =
            GetMessageFieldDesc(desc, TimestampTraits::kSecondsFieldNumber, TimestampTraits::kSecondsFieldType);
        const auto& nanos_desc =
            GetMessageFieldDesc(desc, TimestampTraits::kNanosFieldNumber, TimestampTraits::kNanosFieldType);
        const auto& reflection = *message.GetReflection();

        auto seconds = reflection.GetInt64(message, &seconds_desc);
        const auto nanos = reflection.GetInt32(message, &nanos_desc);

        if (!IsValidTimestamp(seconds, nanos)) {
            throw FieldError(
                PrintErrorCode::kInvalidValue,
                "timestamp's seconds/nanos combination is invalid or represents out of bounds value"
            );
        }

        // ensure that seconds is positive (kMinTimestampSeconds is negative)
        seconds -= kMinTimestampSeconds;

        // Julian Day -> Y/M/D, algorithm from protobuf sources:
        // https://github.com/protocolbuffers/protobuf/blob/v33.2/src/google/protobuf/json/internal/unparser.cc#L612
        // NOLINTNEXTLINE(readability-identifier-naming)
        std::int32_t l = 0;
        std::int32_t n = 0;
        std::int32_t i = 0;
        std::int32_t j = 0;
        std::int32_t k = 0;
        l = static_cast<std::int32_t>(seconds / 86400) - 719162 + 68569 + 2440588;
        n = 4 * l / 146097;
        l = l - (146097 * n + 3) / 4;
        i = 4000 * (l + 1) / 1461001;
        l = l - 1461 * i / 4 + 31;
        j = 80 * l / 2447;
        k = l - 2447 * j / 80;
        l = j / 11;
        j = j + 2 - 12 * l;
        i = 100 * (n - 49) + i + l;

        const std::int32_t sec = seconds % 60;
        const std::int32_t min = (seconds / 60) % 60;
        const std::int32_t hour = (seconds / 3600) % 24;

        std::string value;
        if (nanos == 0) {
            value = fmt::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}Z", i, j, k, hour, min, sec);
        } else {
            std::int32_t seconds_fraction = nanos;  // nanos greater than 0
            int digits = 9;
            while (seconds_fraction % 1000 == 0) {
                seconds_fraction /= 1000;
                digits -= 3;
            }
            value = fmt::format(
                "{:04}-{:02}-{:02}T{:02}:{:02}:{:02}.{:0{}}Z",
                i,
                j,
                k,
                hour,
                min,
                sec,
                seconds_fraction,
                digits
            );
        }

        handler_.String(value);
    }

    void VisitFieldMask(const ::google::protobuf::Message& message) {
        std::string json_paths;
        const auto& reflection = *message.GetReflection();
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            FieldMaskTraits::kPathsFieldNumber,
            FieldMaskTraits::kPathsFieldType,
            true
        );
        const int size = reflection.FieldSize(message, &field_desc);

        for (int i = 0; i < size; ++i) {
            ProtoStringType scratch;
            std::string_view path = reflection.GetRepeatedStringReference(message, &field_desc, i, &scratch);

            std::string json_path;
            json_path.reserve(path.size());
            bool underscore_seen = false;

            for (const char c : path) {
                if (ascii_islower(c)) {
                    json_path.push_back(underscore_seen ? ascii_toupper(c) : c);
                } else if (c == '.' || (ascii_isdigit(c) && !underscore_seen)) {
                    // parser will not be able to restore original path if it contains digit after underscore
                    json_path.push_back(c);
                } else if (c == '_' && !underscore_seen) {
                    underscore_seen = true;
                    continue;
                } else {
                    throw FieldError(PrintErrorCode::kInvalidValue, "field mask path contains unexpected symbol");
                }
                underscore_seen = false;
            }

            if (underscore_seen) {
                // parser will not be able to restore original path if it contains trailing underscore
                throw FieldError(PrintErrorCode::kInvalidValue, "field mask path contains trailing underscore");
            }

            json_paths.append(json_path);
            json_paths.append(1, ',');
        }

        if (!json_paths.empty()) {
            json_paths.pop_back();  // remove trailing ','
        }

        handler_.String(json_paths);
    }

    void VisitValue(const ::google::protobuf::Message& message) {
        const auto& desc = *message.GetDescriptor();
        const auto& reflection = *message.GetReflection();

        {
            const auto& field = GetMessageFieldDesc(desc, ValueTraits::kNullFieldNumber, ValueTraits::kNullFieldType);
            if (reflection.HasField(message, &field)) {
                handler_.Null();
                return;
            }
        }
        {
            const auto&
                field = GetMessageFieldDesc(desc, ValueTraits::kNumberFieldNumber, ValueTraits::kNumberFieldType);
            if (reflection.HasField(message, &field)) {
                const auto value = reflection.GetDouble(message, &field);
                if (std::isnan(value) || std::isinf(value)) {
                    throw FieldError(
                        PrintErrorCode::kInvalidValue,
                        "'google.protobuf.Value' NaN or Inf floating-point value can't be represented in JSON",
                        field.name()
                    );
                }
                handler_.Double(value);
                return;
            }
        }
        {
            const auto&
                field = GetMessageFieldDesc(desc, ValueTraits::kStringFieldNumber, ValueTraits::kStringFieldType);
            if (reflection.HasField(message, &field)) {
                ProtoStringType scratch;
                handler_.String(std::string_view(reflection.GetStringReference(message, &field, &scratch)));
                return;
            }
        }
        {
            const auto& field = GetMessageFieldDesc(desc, ValueTraits::kBoolFieldNumber, ValueTraits::kBoolFieldType);
            if (reflection.HasField(message, &field)) {
                handler_.Bool(reflection.GetBool(message, &field));
                return;
            }
        }
        {
            const auto&
                field = GetMessageFieldDesc(desc, ValueTraits::kStructFieldNumber, ValueTraits::kStructFieldType);
            if (reflection.HasField(message, &field)) {
                try {
                    VisitStruct(reflection.GetMessage(message, &field));
                } catch (FieldError& error) {
                    error.PrependField("struct_value");
                    throw;
                }
                return;
            }
        }
        {
            const auto& field = GetMessageFieldDesc(desc, ValueTraits::kListFieldNumber, ValueTraits::kListFieldType);
            if (reflection.HasField(message, &field)) {
                try {
                    VisitListValue(reflection.GetMessage(message, &field));
                } catch (FieldError& error) {
                    error.PrependField("list_value");
                    throw;
                }
                return;
            }
        }

        // Empty 'google.protobuf.Value' can only occur as a top-level type (otherwise it is skipped during
        // field/repeated/map serialization). Native ProtoJSON converts it to an empty string, but empty string is not
        // valid JSON, so we throw instead.
        throw FieldError(PrintErrorCode::kInvalidValue, "none of the 'google.protobuf.Value' alternatives is set");
    }

    void VisitListValue(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            ListValueTraits::kValuesFieldNumber,
            ListValueTraits::kValuesFieldType,
            true
        );
        VisitRepeated(message, *message.GetReflection(), field_desc);
    }

    void VisitStruct(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            StructTraits::kFieldsFieldNumber,
            StructTraits::kFieldsFieldType,
            true
        );
        UINVARIANT(
            field_desc.is_map(),
            "Well-known message type 'google.protobuf.Struct' field 'fields' should be map"
        );
        VisitMap(message, *message.GetReflection(), field_desc);
    }

    void VisitDoubleValue(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            DoubleValueTraits::kValueFieldNumber,
            DoubleValueTraits::kValueFieldType
        );
        handler_.Double(message.GetReflection()->GetDouble(message, &field_desc));
    }

    void VisitFloatValue(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            FloatValueTraits::kValueFieldNumber,
            FloatValueTraits::kValueFieldType
        );
        handler_.Float(message.GetReflection()->GetFloat(message, &field_desc));
    }

    void VisitInt64Value(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            Int64ValueTraits::kValueFieldNumber,
            Int64ValueTraits::kValueFieldType
        );
        handler_.Int64(message.GetReflection()->GetInt64(message, &field_desc));
    }

    void VisitUInt64Value(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            UInt64ValueTraits::kValueFieldNumber,
            UInt64ValueTraits::kValueFieldType
        );
        handler_.UInt64(message.GetReflection()->GetUInt64(message, &field_desc));
    }

    void VisitInt32Value(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            Int32ValueTraits::kValueFieldNumber,
            Int32ValueTraits::kValueFieldType
        );
        handler_.Int32(message.GetReflection()->GetInt32(message, &field_desc));
    }

    void VisitUInt32Value(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            UInt32ValueTraits::kValueFieldNumber,
            UInt32ValueTraits::kValueFieldType
        );
        handler_.UInt32(message.GetReflection()->GetUInt32(message, &field_desc));
    }

    void VisitBoolValue(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            BoolValueTraits::kValueFieldNumber,
            BoolValueTraits::kValueFieldType
        );
        handler_.Bool(message.GetReflection()->GetBool(message, &field_desc));
    }

    void VisitStringValue(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            StringValueTraits::kValueFieldNumber,
            StringValueTraits::kValueFieldType
        );
        ProtoStringType scratch;
        handler_.String(std::string_view(message.GetReflection()->GetStringReference(message, &field_desc, &scratch)));
    }

    void VisitBytesValue(const ::google::protobuf::Message& message) {
        const auto& field_desc = GetMessageFieldDesc(
            *message.GetDescriptor(),
            BytesValueTraits::kValueFieldNumber,
            BytesValueTraits::kValueFieldType
        );
        ProtoStringType scratch;
        handler_.Bytes(std::string_view(message.GetReflection()->GetStringReference(message, &field_desc, &scratch)));
    }

private:
    Handler& handler_;

    bool always_print_fields_with_no_presence_{false};
    bool always_print_enums_as_ints_{false};
    bool preserve_proto_field_names_{false};
    bool expand_any_{false};
    bool expand_any_fallback_to_raw_{false};
    bool redact_debug_string_{false};
};

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
