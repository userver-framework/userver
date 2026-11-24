#pragma once

#include <functional>
#include <utility>
#include <variant>

#include <userver/protobuf/arena.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @brief Owns a message by value or explicitly borrows an arena-backed protobuf message.
/// Borrowing preserves message identity and avoids copying protobuf data off its arena.
/// The arena must remain alive and must not be reset while the stored message is in use.
/// Copying or moving the storage preserves borrowing; ownership of the arena is never transferred.
template <typename Message>
class MessageStorage final {
public:
    /// @brief Creates an owned, default-constructed message.
    MessageStorage() = default;

    /// @brief Moves the message into owned storage, copying arena-backed data off its arena.
    explicit MessageStorage(Message&& message)
        : message_(std::in_place_type<Message>, std::move(message))
    {}

    /// @brief Borrows a message without taking ownership of its arena.
    explicit MessageStorage(std::reference_wrapper<Message> message)
        : message_(message)
    {}

    static MessageStorage AllocateOnArena(google::protobuf::Arena& arena) {
        return MessageStorage{std::ref(*protobuf::CreateArenaMessage<Message>(arena))};
    }

    Message& Get() noexcept {
        return std::visit([](Message& message) -> auto& { return message; }, message_);
    }

    const Message& Get() const noexcept {
        return std::visit([](const Message& message) -> auto& { return message; }, message_);
    }

private:
    std::variant<Message, std::reference_wrapper<Message>> message_;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
