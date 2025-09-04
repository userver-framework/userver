#pragma once

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class EventBase {
public:
    virtual ~EventBase() = default;

    /// @brief For use from the blocking call queue
    /// @param `bool ok` returned by `grpc::CompletionQueue::Next`
    virtual void Notify(bool ok) noexcept = 0;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
