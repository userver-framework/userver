#pragma once

/// @file userver/protobuf/string.hpp
/// @brief Operations with Protobuf string type.

#if defined(ARCADIA_ROOT)
#include <google/protobuf/port.h>
USERVER_NAMESPACE_BEGIN

namespace protobuf {
using ProtoStringType = TProtoStringType;
}  // namespace protobuf
USERVER_NAMESPACE_END
#else
#include <string>
using ProtoStringType = std::string;
#endif
