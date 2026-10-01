#pragma once

#include <type_traits>
#include <utility>

#include <grpcpp/server_context.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/server_callback.h>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/impl/event_base.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

template <typename CallbackTraits>
class CallbackResponder final {
    using SerializedFinalResponse = typename CallbackTraits::SerializedFinalResponse;
    using Reactor = typename CallbackTraits::ReactorBase;

public:
    CallbackResponder(
        Reactor& reactor,
        const grpc::ServerContextBase& server_context,
        SerializedFinalResponse& final_response
    )
        : reactor_{reactor},
          server_context_{server_context}
    {
        if constexpr (!std::is_same_v<SerializedFinalResponse, NoFinalResponse>) {
            unary_response_ = &final_response;
        }
    }

    bool IsCancelled() const { return server_context_.IsCancelled(); }

    void Finish(grpc::ByteBuffer&& response, const grpc::Status& status) {
        UASSERT(unary_response_);
        unary_response_->Swap(&response);
        reactor_.Finish(status);
    }

    void Finish(const grpc::Status& status) { reactor_.Finish(status); }

    void FinishWithError(const grpc::Status& status) {
        UASSERT(!status.ok());
        reactor_.Finish(status);
    }

    void WriteAndFinish(grpc::ByteBuffer&& response, grpc::WriteOptions options, const grpc::Status& status) {
        write_and_finish_response_storage_.Swap(&response);
        reactor_.StartWriteAndFinish(&write_and_finish_response_storage_, options, status);
    }

    void Read(grpc::ByteBuffer* request, void* tag) {
        UASSERT(!read_event_);
        read_event_ = static_cast<ugrpc::impl::EventBase*>(tag);
        reactor_.StartRead(request);
    }

    void Write(const grpc::ByteBuffer& response, grpc::WriteOptions options, void* tag) {
        UASSERT(!write_event_);
        write_event_ = static_cast<ugrpc::impl::EventBase*>(tag);
        // Responder always waits for the tag immediately, even on cancellations,
        // so `response` is alive until OnWriteDone.
        reactor_.StartWrite(&response, options);
    }

    void NotifyRead(bool ok) noexcept {
        auto* const event = std::exchange(read_event_, nullptr);
        UASSERT(event);
        event->Notify(ok);
    }

    void NotifyWrite(bool ok) noexcept {
        auto* const event = std::exchange(write_event_, nullptr);
        UASSERT(event);
        event->Notify(ok);
    }

private:
    Reactor& reactor_;
    const grpc::ServerContextBase& server_context_;
    grpc::ByteBuffer* unary_response_{};
    grpc::ByteBuffer write_and_finish_response_storage_;
    ugrpc::impl::EventBase* read_event_{};
    ugrpc::impl::EventBase* write_event_{};
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
