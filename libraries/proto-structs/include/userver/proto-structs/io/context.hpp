#pragma once

/// @file userver/proto-structs/io/context.hpp
/// @brief Conversion context classes which provide methods for performing conversion.

#include <limits>
#include <type_traits>
#include <utility>

#include <userver/proto-structs/io/context_base.hpp>
#include <userver/proto-structs/io/fwd.hpp>
#include <userver/proto-structs/io/std/scalar.hpp>
#include <userver/proto-structs/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

template <traits::ProtoMessage TMessage, traits::ProtoStruct TStruct>
void MessageToStruct(const TMessage&, TStruct&);

template <typename TStruct, traits::ProtoMessage TMessage>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
void StructToMessage(TStruct&&, TMessage&);

}  // namespace proto_structs

namespace proto_structs::io {

/// @brief Read operation context passed to user-defined `ReadProtoStruct` global functions.
class ReadContext final : public ContextWithErrors<ReadError> {
private:
    using ContextWithErrors<ReadError>::ContextWithErrors;
    using ContextWithErrors<ReadError>::PushToPath;
    using ContextWithErrors<ReadError>::PopFromPath;

    template <proto_structs::traits::ProtoMessage TMessage, proto_structs::traits::ProtoStruct TStruct>
    friend void proto_structs::MessageToStruct(const TMessage&, TStruct&);

public:
    /// @brief Converts protobuf message field @a message_field with number @a field_number to a proto struct field
    ///        with type `T`.
    /// @tparam T proto struct field type
    /// @tparam U protobuf message type
    template <typename T, typename U>
    T ReadField(int field_number, const U& message_field) {
        if constexpr (requires { ReadProtoField(*this, To<std::remove_cv_t<T>>{}, field_number, message_field); }) {
            return ReadProtoField(*this, To<std::remove_cv_t<T>>{}, field_number, message_field);
        } else {
            static_assert(
                sizeof(T) && false,
                "There is no 'ReadProtoField(proto_structs::io::ReadContext&, proto_structs::io::To<T>, int, const U&) "
                "in namespace 'proto_structs::io'. Did you forget to include 'conv'-header responsible for reading 'T' "
                "from 'userver/proto-structs/io'?"
            );
            return {};
        }
    }

    /// @brief Converts protobuf message field @a message_field with number @a field_number to a proto struct field
    ///        with type `T`.
    /// @tparam T proto struct field type
    /// @tparam U protobuf message type
    template <proto_structs::traits::ProtoStruct TStruct, proto_structs::traits::ProtoMessage TMessage>
    TStruct ReadField(int field_number, const TMessage& message_field) {
        static_assert(
            proto_structs::traits::CompatibleWith<TStruct, TMessage>,
            "proto struct type is not compatible with protobuf message type"
        );

        if constexpr (requires { ReadProtoStruct(*this, To<std::remove_cv_t<TStruct>>{}, message_field); }) {
            // Note, that not popping field_number in case of exception thrown from the `ReadProtoStruct` is
            // intentional: this allows us later to catch this exception and convert it to `ReadError` with
            // complete information about the field which was not converted.

            PushToPath(field_number);
            auto obj = ReadProtoStruct(*this, To<std::remove_cv_t<TStruct>>{}, message_field);
            PopFromPath();
            return obj;
        } else {
            static_assert(
                sizeof(TStruct) && false,
                "There is no 'ReadProtoStruct(proto_structs::io::ReadContext&, proto_structs::io::To<TStruct>, int, "
                "const TMessage&) in namespace of 'TStruct' or 'proto_structs::io'. Probably you have not provided "
                "'ReadProtoStruct' function overload or have not included 'conv'-header responsible for reading "
                "well-known 'TStruct'."
            );
            return {};
        }
    }

    /// @brief Converts protobuf message field @a message_field with number @a field_number to the proto struct field
    ///        @a struct_field.
    /// @tparam T proto struct field type
    /// @tparam U protobuf message type
    template <typename T, typename U>
    void ReadField(T& struct_field, int field_number, const U& message_field) {
        struct_field = ReadContext::ReadField<T>(field_number, message_field);
    }
};

/// @brief Write operation context passed to user-defined `WriteProtoStruct` global functions.
class WriteContext final : public ContextWithErrors<WriteError> {
protected:
    using ContextWithErrors<WriteError>::ContextWithErrors;
    using ContextWithErrors<WriteError>::PushToPath;
    using ContextWithErrors<WriteError>::PopFromPath;

    template <typename TStruct, proto_structs::traits::ProtoMessage TMessage>
    requires proto_structs::traits::ProtoStruct<std::remove_cvref_t<TStruct>>
    friend void proto_structs::StructToMessage(TStruct&&, TMessage&);

public:
    /// @brief Converts proto struct field @a struct_field to the protobuf message field @a message_field with number
    ///        @a field_number.
    /// @tparam T proto struct field type
    /// @tparam U protobuf message type
    template <typename T, typename U>
    void WriteField(T&& struct_field, int field_number, U& message_field) {
        if constexpr (requires { WriteProtoField(*this, std::forward<T>(struct_field), field_number, message_field); })
        {
            WriteProtoField(*this, std::forward<T>(struct_field), field_number, message_field);
        } else {
            static_assert(
                sizeof(T) && false,
                "There is no 'WriteProtoField(proto_structs::io::WriteContext&, T&&, int, U&) in namespace "
                "'proto_structs::io'. Did you forget to include 'conv'-header responsible for writing 'T' from "
                "'userver/proto-structs/io'?"
            );
        }
    }

    /// @brief Converts proto struct field @a struct_field to the protobuf message field @a message_field with number
    ///        @a field_number.
    /// @tparam T proto struct field type
    /// @tparam U protobuf message type
    template <typename TStruct, proto_structs::traits::ProtoMessage TMessage>
    requires proto_structs::traits::ProtoStruct<std::remove_cvref_t<TStruct>>
    void WriteField(TStruct&& struct_field, int field_number, TMessage& message_field) {
        static_assert(
            proto_structs::traits::CompatibleWith<std::remove_cvref_t<TStruct>, TMessage>,
            "proto struct type is not compatible with protobuf message type"
        );

        if constexpr (requires { WriteProtoStruct(*this, std::forward<TStruct>(struct_field), message_field); }) {
            // Note, that not popping field_number in case of exception thrown from the `WriteProtoStruct` is
            // intentional: this allows us later to catch this exception and convert it to `WriteError` with
            // complete information about the field which was not converted.

            PushToPath(field_number);
            WriteProtoStruct(*this, std::forward<TStruct>(struct_field), message_field);
            PopFromPath();
        } else {
            static_assert(
                sizeof(TStruct) && false,
                "There is no 'WriteProtoStruct(proto_structs::io::WriteContext&, TStruct&&, TMessage&) in namespace of "
                "'TStruct' or 'proto_structs::io'. Probably you have not provided 'WriteProtoStruct' function overload "
                "or have not included 'conv'-header responsible for writing well-known 'TStruct'."
            );
        }
    }
};

}  // namespace proto_structs::io

namespace proto_structs::io::impl {

// clang-format off

template<typename TStdType, typename TProtobufType>
concept SignCompatibleWith =
    (std::is_signed_v<std::remove_cv_t<TStdType>> && std::is_signed_v<std::remove_cv_t<TProtobufType>>)
    ||
    (std::is_unsigned_v<std::remove_cv_t<TStdType>> && std::is_unsigned_v<std::remove_cv_t<TProtobufType>>);

template<typename TStdType, typename TProtobufType>
concept IntegerCompatibleWith =
    (std::is_same_v<std::remove_cv_t<TStdType>, std::int32_t> ||
     std::is_same_v<std::remove_cv_t<TStdType>, std::int64_t> ||
     std::is_same_v<std::remove_cv_t<TStdType>, std::uint32_t> ||
     std::is_same_v<std::remove_cv_t<TStdType>, std::uint64_t>)
    &&
    sizeof(TStdType) == sizeof(TProtobufType)
    &&
    SignCompatibleWith<TStdType, TProtobufType>;

template<typename TStdType, typename TProtobufType>
concept StringCompatibleWith =
    std::is_same_v<std::remove_cv_t<TStdType>, std::string>
    &&
    std::is_constructible_v<std::string, TProtobufType>;

template<typename TStdType, typename TProtobufType>
concept EnumCompatibleWith =
    std::is_enum_v<TStdType>
    &&
    (std::is_enum_v<TProtobufType> || std::is_integral_v<TProtobufType>);

template<typename TStdType, typename TProtobufType>
concept SizeTypeCompatibleWith =
    std::is_same_v<std::remove_cv_t<TStdType>, std::size_t> && std::is_integral_v<TProtobufType>;

// clang-format on

}  // namespace proto_structs::io::impl

namespace proto_structs::io {

/// @brief Determines compatability between protobuf/std scalar types.
template <typename TStdType, typename TProtobufType>
concept ScalarCompatibleWith =
    traits::ProtoScalar<TStdType> &&
    (std::is_same_v<std::remove_cv_t<TStdType>, std::remove_cv_t<TProtobufType>> ||
     impl::IntegerCompatibleWith<TStdType, TProtobufType> || impl::StringCompatibleWith<TStdType, TProtobufType> ||
     impl::EnumCompatibleWith<TStdType, TProtobufType> || impl::SizeTypeCompatibleWith<TStdType, TProtobufType>);

template <typename TStructField, typename TMessageField>
requires ScalarCompatibleWith<TStructField, TMessageField>
TStructField ReadProtoField(ReadContext& ctx, To<TStructField>, int field_number, const TMessageField& message_field) {
    if constexpr (std::is_same_v<std::size_t, TStructField>) {
        if (message_field >= 0 && static_cast<std::uintmax_t>(message_field) <= std::numeric_limits<std::size_t>::max())
        {
            return static_cast<std::size_t>(message_field);
        } else {
            ctx.AddError(field_number, "value is out of range");
            return {};
        }
    } else {
        return static_cast<TStructField>(message_field);
    }
}

template <typename TStructField, typename TMessageField>
requires ScalarCompatibleWith<std::remove_cvref_t<TStructField>, TMessageField>
void WriteProtoField(WriteContext& ctx, TStructField&& struct_field, int field_number, TMessageField& message_field) {
    if constexpr (std::is_same_v<std::remove_cvref_t<TStructField>, std::size_t>) {
        if (struct_field <= std::numeric_limits<TMessageField>::max()) {
            message_field = static_cast<TMessageField>(struct_field);
        } else {
            ctx.AddError(field_number, "value is out of range");
        }
    } else if constexpr (std::is_enum_v<std::remove_cvref_t<TStructField>>) {
        message_field = static_cast<std::remove_cv_t<TMessageField>>(struct_field);
    } else {
        message_field = std::forward<TStructField>(struct_field);
    }
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
