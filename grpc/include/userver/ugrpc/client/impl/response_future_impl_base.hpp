#pragma once

#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/impl/context_accessor.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {
class CallContext;
}  // namespace ugrpc::client

namespace ugrpc::client::impl {

template <typename Response>
class ResponseFutureImplBase {
public:
    virtual ~ResponseFutureImplBase() = default;

    virtual bool IsReady() const = 0;

    virtual engine::FutureStatus WaitUntil(engine::Deadline deadline) const noexcept = 0;

    virtual Response Get() = 0;

    virtual CallContext& GetContext() noexcept = 0;
    virtual const CallContext& GetContext() const noexcept = 0;

    virtual engine::impl::ContextAccessor* TryGetContextAccessor() noexcept = 0;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
