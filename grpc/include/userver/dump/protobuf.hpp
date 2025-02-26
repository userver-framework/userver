#pragma once

/// @file userver/dump/protobuf.hpp
/// @brief Dumping support for protobuf messages
///
/// @ingroup userver_dump_read_write

#include <type_traits>

#include <google/protobuf/message_lite.h>

#include <userver/dump/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace impl {

void WriteProtoMessageToDump(Writer& writer, const google::protobuf::MessageLite& message);

void ParseProtoMessageFromDump(Reader& reader, google::protobuf::MessageLite& message);

}  // namespace impl

/// @brief Protobuf message dumping support
template <typename T>
std::enable_if_t<std::is_base_of_v<google::protobuf::MessageLite, T>> Write(Writer& writer, const T& value) {
    impl::WriteProtoMessageToDump(writer, value);
}

/// @brief Protobuf message dumping support
template <typename T>
std::enable_if_t<std::is_base_of_v<google::protobuf::MessageLite, T>, T> Read(Reader& reader, To<T>) {
    T value;
    impl::ParseProtoMessageFromDump(reader, value);
    return value;
}

}  // namespace dump

USERVER_NAMESPACE_END
