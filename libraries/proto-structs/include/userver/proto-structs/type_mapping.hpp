#pragma once

/// @file userver/proto-structs/type_mapping.hpp
/// @brief Concepts and traits for checking struct and protobuf message compatability

#include <type_traits>

namespace google::protobuf {
class Message;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for the proto-structs library
namespace proto_structs {

/// @brief Namespace contains types for obtaining meta information (traits, concepts, etc.)
namespace traits {

/// @brief Struct concept
template <typename T>
concept ProtoStruct = std::is_class_v<T> && std::is_aggregate_v<T>;

/// @brief `protoc`-generated message concept
template <typename T>
concept ProtoMessage = std::is_base_of_v<::google::protobuf::Message, T> && !
std::is_same_v<::google::protobuf::Message, std::remove_cv_t<T>>;

/// @brief Trait that marks struct as compatible to some protobuf message
/// @tparam TStruct struct type
/// By default, trait returns `TStruct::ProtobufMessage` as it's `Type` definition. User can also specialize
/// `CompatibleMessageTrait` for any struct type in the `proto_structs::traits` namespace if he can not modify the
/// code which contains `TStruct` definition:
/// ```cpp
/// namespace proto_structs::traits {
///   template<>
///   struct CompatibleMessageTrait<ExternalStruct> {
///     using Type = SomeProtoMessage;
///   }
/// }
/// ```
template <traits::ProtoStruct TStruct>
struct CompatibleMessageTrait {
    using Type = typename TStruct::ProtobufMessage;
    static_assert(ProtoMessage<Type>, "'ProtobufMessage' does not represent protoc-generatated message type");
};

template <traits::ProtoStruct TStruct>
using CompatibleMessageType = typename CompatibleMessageTrait<std::remove_cv_t<TStruct>>::Type;

/// @brief Describes struct type for which compatability information is properly provided.
template <typename T>
concept CompatibleStruct = requires {
                               requires traits::ProtoStruct<T>;
                               typename CompatibleMessageType<T>;
                           };
}  // namespace traits

/// @brief An ADL helper which allows locating functions to read structs from protobuf messages inside struct-specific
///        namespaces
template <typename T>
struct To {
    using Type = T;
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
