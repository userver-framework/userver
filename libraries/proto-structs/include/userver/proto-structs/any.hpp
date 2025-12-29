#pragma once

/// @file userver/proto-structs/any.hpp
/// @brief @copybrief proto_structs::Any

#include <type_traits>
#include <utility>

#include <google/protobuf/any.pb.h>

#include <userver/proto-structs/convert.hpp>
#include <userver/proto-structs/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Wrapper for `google.protobuf.Any` which provides interface to access stored message as compatible struct.
class Any final {
public:
    using ProtobufMessage = ::google::protobuf::Any;

    /// @brief Creates empty `Any`.
    Any() noexcept = default;

    /// @brief Creates wrapper initializing its underlying storage with @a proto_any.
    Any(google::protobuf::Any proto_any)
        : storage_(std::move(proto_any))
    {}

    /// @brief Creates `Any` holding @a obj.
    /// @tparam TStruct proto struct type
    /// @throws WriteError if conversion of @a obj to its compatible message has failed.
    /// @throws AnyPackError if compatible protobuf message can not be packed to `google.protobuf.Any`.
    template <typename TStruct>
    requires(!std::is_same_v<std::remove_cvref_t<TStruct>, ::google::protobuf::Any>) &&
            (!std::is_same_v<std::remove_cvref_t<TStruct>, Any>) && traits::ProtoStruct<std::remove_cvref_t<TStruct>>
    Any(TStruct&& obj) {
        Pack(std::forward<TStruct>(obj));
    }

    /// @brief Packs @a obj in `Any`.
    /// @tparam TStruct proto struct type
    /// @throws WriteError if conversion of @a obj to its compatible message has failed.
    /// @throws AnyPackError if compatible protobuf message can not be packed to `google.protobuf.Any`.
    template <typename TStruct>
    requires(!std::is_same_v<std::remove_cvref_t<TStruct>, ::google::protobuf::Any>) &&
            (!std::is_same_v<std::remove_cvref_t<TStruct>, Any>) && traits::ProtoStruct<std::remove_cvref_t<TStruct>>
    Any& operator=(TStruct&& obj) {
        Pack(std::forward<TStruct>(obj));
        return *this;
    }

    /// @brief Returns `true` if `Any` contains `TStruct`.
    /// @tparam TStruct proto struct type
    template <traits::ProtoStruct TStruct>
    [[nodiscard]] bool Is() const noexcept {
        using Message = traits::CompatibleMessageType<TStruct>;
        return storage_.Is<Message>();
    }

    /// @brief Returns `true` if underlying `google.protobuf.Any` contains `TMessage`.
    /// @tparam TMessage protobuf message type
    template <traits::ProtoMessage TMessage>
    [[nodiscard]] bool Is() const noexcept {
        return storage_.Is<std::remove_cv_t<TMessage>>();
    }

    /// @brief Unpacks `Any` to `TStruct` struct.
    /// @tparam TStruct proto struct type
    /// @throws AnyUnpackError if underlying `google.protobuf.Any` does not contain message compatible to `TStruct`.
    /// @throws ReadError if conversion of unpacked protobuf message to proto struct has failed.
    template <traits::ProtoStruct TStruct>
    [[nodiscard]] TStruct Unpack() {
        using Message = traits::CompatibleMessageType<TStruct>;
        return MessageToStruct<TStruct>(Unpack<Message>());
    }

    /// @brief Unpacks underlying `google.protobuf.Any` to `TMessage` message.
    /// @tparam TMessage protobuf message type
    /// @throws AnyUnpackError if underlying `google.protobuf.Any` does not contain `TMessage` type message.
    template <traits::ProtoMessage TMessage>
    [[nodiscard]] TMessage Unpack() {
        TMessage msg;

        if (!storage_.UnpackTo(&msg)) {
            throw AnyUnpackError(TMessage::descriptor()->full_name());
        }

        return msg;
    }

    /// @brief Packs @a obj to `Any`.
    /// @tparam TStruct proto struct type
    /// @throws WriteError if conversion of @a obj to its compatible message has failed.
    /// @throws AnyPackError if packing of compatible protobuf message to `google.protobuf.Any` has failed.
    template <typename TStruct>
    requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
    void Pack(TStruct&& obj) {
        using Message = traits::CompatibleMessageType<std::remove_cvref_t<TStruct>>;
        Pack<Message>(StructToMessage(std::forward<TStruct>(obj)));
    }

    /// @brief Packs @a message to underlying `google.protobuf.Any`.
    /// @tparam TMessage protobuf message type
    /// @throws AnyPackError if packing of protobuf message to `google.protobuf.Any` has failed.
    template <traits::ProtoMessage TMessage>
    void Pack(const TMessage& message) {
        if (!storage_.PackFrom(message)) {
            throw AnyUnpackError(TMessage::descriptor()->full_name());
        }
    }

    /// @brief Returns underlying `google.protobuf.Any`.
    [[nodiscard]] const ::google::protobuf::Any& GetProtobufAny() const& noexcept { return storage_; }

    /// @brief Returns underlying `google.protobuf.Any`.
    [[nodiscard]] ::google::protobuf::Any&& GetProtobufAny() && noexcept { return std::move(storage_); }

private:
    ::google::protobuf::Any storage_;
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
