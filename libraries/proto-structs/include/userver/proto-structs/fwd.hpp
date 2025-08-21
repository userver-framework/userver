#pragma once

/// @file
/// @brief Forward declarations of types in userver proto-structs library.

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl {

class ReadContext;
class WriteContext;

}  // namespace proto_structs::impl

namespace proto_structs {

class Any;

template <typename T>
struct To;

}  // namespace proto_structs

USERVER_NAMESPACE_END
