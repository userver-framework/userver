#pragma once

#include <memory>

#include <grpcpp/server_context.h>

#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

struct ContextIntrusiveDeleter final {
    void operator()(grpc::CallbackServerContext* context) const noexcept;
    void operator()(grpc::GenericCallbackServerContext* context) const noexcept;
};

template <typename ContextType>
using ContextIntrusiveRef = utils::NotNull<std::unique_ptr<ContextType, ContextIntrusiveDeleter>>;

ContextIntrusiveRef<grpc::CallbackServerContext> ContextAddRef(grpc::CallbackServerContext& context) noexcept;

ContextIntrusiveRef<grpc::GenericCallbackServerContext> ContextAddRef(grpc::GenericCallbackServerContext& context
) noexcept;

// The allocator lets userver extend callback ServerContext lifetime past gRPC's
// OnDone, at least until MiddlewareBase::OnCallFinish hooks are called.
class ContextAllocator final : public grpc::ContextAllocator {
public:
    grpc::CallbackServerContext* NewCallbackServerContext() override;

    grpc::GenericCallbackServerContext* NewGenericCallbackServerContext() override;

    void Release(grpc::CallbackServerContext* context) override;

    void Release(grpc::GenericCallbackServerContext* context) override;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
