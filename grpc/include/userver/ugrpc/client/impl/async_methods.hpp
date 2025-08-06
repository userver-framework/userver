#pragma once

#include <type_traits>

#include <google/protobuf/message.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <typename Message>
const google::protobuf::Message* ToBaseMessage(const Message* message) {
    if constexpr (std::is_base_of_v<google::protobuf::Message, Message>) {
        return message;
    } else {
        return nullptr;
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
