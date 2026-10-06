#include <userver/ugrpc/server/impl/context_allocator.hpp>

#include <atomic>

#include <grpcpp/generic/async_generic_service.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

template <typename ContextType>
class OwnedContext final {
public:
    ContextType& GetContext() noexcept { return context_; }

    void AddRef() noexcept { refcount_.fetch_add(1, std::memory_order_relaxed); }

    void ReleaseRef() noexcept {
        if (refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }

private:
    ContextType context_;
    std::atomic<std::size_t> refcount_{1};
};

template <typename ContextType>
OwnedContext<ContextType>& AsOwnedContext(ContextType& context) noexcept {
    // Technically, converting a pointer to the first field back to the
    // containing object via reinterpret_cast is UB. In practice compilers
    // understand this trick, and it works on all supported platforms. We need
    // this because grpc::GenericCallbackServerContext is final.
    return reinterpret_cast<OwnedContext<ContextType>&>(context);
}

template <typename ContextType>
ContextIntrusiveRef<ContextType> DoContextAddRef(ContextType& context) noexcept {
    auto& owned_context = AsOwnedContext(context);
    owned_context.AddRef();
    return ContextIntrusiveRef<ContextType>{std::unique_ptr<ContextType, ContextIntrusiveDeleter>(&context)};
}

template <typename ContextType>
void ReleaseContext(ContextType& context) noexcept {
    AsOwnedContext(context).ReleaseRef();
}

}  // namespace

void ContextIntrusiveDeleter::operator()(grpc::CallbackServerContext* context) const noexcept {
    ReleaseContext(*context);
}

void ContextIntrusiveDeleter::operator()(grpc::GenericCallbackServerContext* context) const noexcept {
    ReleaseContext(*context);
}

ContextIntrusiveRef<grpc::CallbackServerContext> ContextAddRef(grpc::CallbackServerContext& context) noexcept {
    return DoContextAddRef(context);
}

ContextIntrusiveRef<grpc::GenericCallbackServerContext> ContextAddRef(grpc::GenericCallbackServerContext& context
) noexcept {
    return DoContextAddRef(context);
}

grpc::CallbackServerContext* ContextAllocator::NewCallbackServerContext() {
    auto* const owned_context = new OwnedContext<grpc::CallbackServerContext>;
    return &owned_context->GetContext();
}

grpc::GenericCallbackServerContext* ContextAllocator::NewGenericCallbackServerContext() {
    auto* const owned_context = new OwnedContext<grpc::GenericCallbackServerContext>;
    return &owned_context->GetContext();
}

void ContextAllocator::Release(grpc::CallbackServerContext* context) {
    if (context == nullptr) {
        UASSERT(false);
        return;
    }

    // Note: context might be a grpc::GenericCallbackServerContext.
    if (auto* const generic_context = dynamic_cast<grpc::GenericCallbackServerContext*>(context)) {
        ReleaseContext(*generic_context);
    } else {
        ReleaseContext(*context);
    }
}

void ContextAllocator::Release(grpc::GenericCallbackServerContext* context) { ReleaseContext(*context); }

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
