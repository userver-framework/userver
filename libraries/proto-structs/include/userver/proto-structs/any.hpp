#pragma once

/// @file userver/proto-structs/any.hpp
/// @brief Class to access `google.protobuf.Any` stored message as a struct

#include <optional>
#include <type_traits>
#include <utility>

#include <google/protobuf/any.pb.h>

#include <userver/proto-structs/convert.hpp>
#include <userver/proto-structs/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Wrapper for `google.protobuf.Any` which provides interface to access stored message as compatible struct
class Any final {
public:
    /// @brief Creates empty `Any`.
    Any() noexcept = default;

    /// @brief Creates wrapper initializing it's underlying storage with @a proto_any
    Any(google::protobuf::Any proto_any) : storage_(std::move(proto_any)) {}

    /// @brief Creates `Any` holding @a obj
    /// @tparam TStruct struct with defined compatability
    /// @throws ConversionError if @a obj can't be converted to it's compatible protobuf message
    /// @throws AnyPackError if compatible protobuf message can not be packed to `google.protobuf.Any`
    /// @note This overload only works for structs for which compatability is defined using
    ///       @ref proto_structs::compatible_message
    template <typename TStruct>
    requires traits::CompatibleStruct<std::remove_cvref_t<TStruct>>
    Any(TStruct&& obj) {
        Pack(std::forward<TStruct>(obj));
    }

    /// @brief Packs @a obj in `Any`
    /// @tparam TStruct struct with defined compatability
    /// @throws ConversionError if @a obj can't be converted to it's compatible protobuf message
    /// @throws AnyPackError if compatible protobuf message can not be packed to `google.protobuf.Any`
    /// @note This overload only works for structs for which compatability is defined using
    ///       @ref proto_structs::compatible_message
    /// @note Method caches @a obj and later calls to @ref Get will not trigger unpacking
    template <typename TStruct>
    requires traits::CompatibleStruct<std::remove_cvref_t<TStruct>>
    Any& operator=(TStruct&& obj) {
        Pack(std::forward<TStruct>(obj));
        return *this;
    }

    /// @brief Returns `true` if `Any` contains `TStruct`
    /// @tparam TStruct struct with defined compatability
    /// @note This method only works for structs for which compatability is defined using
    ///       @ref proto_structs::compatible_message
    template <traits::CompatibleStruct TStruct>
    bool Is() const noexcept {
        using Message = traits::CompatibleMessageType<TStruct>;
        return storage_.Is<Message>();
    }

    /// @brief Returns `true` if underlying `google.protobuf.Any` contains `TMessage`
    /// @tparam TMessage protobuf message type
    template <traits::ProtoMessage TMessage>
    bool Is() const noexcept {
        return storage_.Is<std::remove_cv_t<TMessage>>();
    }

    /// @brief Unpacks `Any` to `TStruct` struct
    /// @tparam TStruct struct with defined compatability
    /// @throws AnyUnpackError if underlying `google.protobuf.Any` does not contain message compatible to `TStruct`
    /// @throws ConversionError if struct can be converted from it's compatible message
    /// @note This method only works for structs for which compatability is defined using
    ///       @ref proto_structs::compatible_message
    template <traits::CompatibleStruct TStruct>
    TStruct Unpack() {
        using Message = traits::CompatibleMessageType<TStruct>;
        return MessageToStruct<TStruct>(Unpack<Message>());
    }

    /// @brief Unpacks underlying `google.protobuf.Any` to `TMessage` message
    /// @tparam TMessage protobuf message type
    /// @throws AnyUnpackError if underlying `google.protobuf.Any` does not contain `TMessage` type message
    template <traits::ProtoMessage TMessage>
    TMessage Unpack() {
        TMessage msg;

        if (!storage_.UnpackTo(&msg)) {
            throw AnyUnpackError(TMessage::descriptor()->full_name());
        }

        return msg;
    }

    /// @brief Packs @a obj to `Any`
    /// @tparam TStruct struct with defined compatability
    /// @throws ConversionError if @a obj can't be converted to it's compatible protobuf message
    /// @throws AnyPackError if packing of compatible protobuf message to `google.protobuf.Any` had failed
    /// @note This method only works for structs for which compatability is defined using
    ///       @ref proto_structs::compatible_message
    /// @note Method caches @a obj and later calls to @ref Get will not trigger unpacking
    template <typename TStruct>
    requires traits::CompatibleStruct<std::remove_cvref_t<TStruct>>
    void Pack(TStruct&& obj) {
        using Message = traits::CompatibleMessageType<std::remove_cvref_t<TStruct>>;
        Pack<Message>(StructToMessage(std::forward<TStruct>(obj)));
    }

    /// @brief Packs @a message to underlying `google.protobuf.Any`
    /// @tparam TMessage protobuf message type
    /// @throws AnyPackError if packing of protobuf message to `google.protobuf.Any` had failed
    template <traits::ProtoMessage TMessage>
    void Pack(const TMessage& message) {
        if (!storage_.PackFrom(&message)) {
            throw AnyUnpackError(TMessage::descriptor()->full_name());
        }
    }

    /// @brief Returns underlying `google.protobuf.Any`
    const ::google::protobuf::Any& GetProtobufAny() const& noexcept { return storage_; }

    /// @brief Returns underlying `google.protobuf.Any`
    ::google::protobuf::Any GetProtobufAny() && noexcept { return std::move(storage_); }

private:
    ::google::protobuf::Any storage_;
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
