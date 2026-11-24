#pragma once

/// @file userver/protobuf/arena.hpp
/// @brief Protobuf arena allocation utilities.

#include <google/protobuf/arena.h>
#include <google/protobuf/stubs/common.h>

#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf {

/// @brief Allocate a protobuf message on an arena across supported protobuf versions.
/// @tparam Message A concrete protobuf message type supporting arena allocation.
/// @param arena The arena that owns the message.
/// @returns a non-owning, non-null pointer to the new message. The pointer becomes invalid
/// when the arena is reset or destroyed; it must not outlive the arena.
template <typename Message>
[[nodiscard]] utils::NotNull<Message*> CreateArenaMessage(google::protobuf::Arena& arena) {
// Protobuf 6.30 removed CreateMessage; older Create does not propagate the arena to message fields.
#if GOOGLE_PROTOBUF_VERSION >= 6030000
    return utils::NotNull<Message*>{google::protobuf::Arena::Create<Message>(&arena)};
#else
    return utils::NotNull<Message*>{google::protobuf::Arena::CreateMessage<Message>(&arena)};
#endif
}

}  // namespace protobuf

USERVER_NAMESPACE_END
