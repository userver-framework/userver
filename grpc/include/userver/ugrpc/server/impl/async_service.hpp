#pragma once

#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/server_context.h>

#include <userver/ugrpc/server/impl/call_traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

template <typename Service>
class AsyncService final : public Service::Service {
 public:
  explicit AsyncService(std::size_t method_count) {
    // Mark all methods as implemented
    for (std::size_t i = 0; i < method_count; ++i) {
      this->MarkMethodAsync(i);
    }
  }

  template <typename CallTraits>
  void Prepare(int method_id, grpc::ServerContext& context,
               typename CallTraits::InitialRequest& initial_request,
               typename CallTraits::RawCall& stream,
               grpc::CompletionQueue& call_cq,
               grpc::ServerCompletionQueue& notification_cq, void* tag) {
    constexpr auto kCallCategory = CallTraits::kCallCategory;

    if constexpr (kCallCategory == CallCategory::kUnary) {
      this->RequestAsyncUnary(method_id, &context, &initial_request, &stream,
                              &call_cq, &notification_cq, tag);
    } else if constexpr (kCallCategory == CallCategory::kInputStream) {
      this->RequestAsyncClientStreaming(method_id, &context, &stream, &call_cq,
                                        &notification_cq, tag);
    } else if constexpr (kCallCategory == CallCategory::kOutputStream) {
      this->RequestAsyncServerStreaming(method_id, &context, &initial_request,
                                        &stream, &call_cq, &notification_cq,
                                        tag);
    } else {
      this->RequestAsyncBidiStreaming(method_id, &context, &stream, &call_cq,
                                      &notification_cq, tag);
    }
  }
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
