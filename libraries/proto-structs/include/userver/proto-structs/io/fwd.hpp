#pragma once

/// @file userver/proto-structs/io/fwd.hpp
/// @brief Forward declarations of utlity types for conversions.

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

/// @brief An ADL helper which allows compiler to find functions to read proto structs from protobuf messages.
template <typename T>
struct To {
    using Type = T;
};

class ReadContext;
class WriteContext;

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
