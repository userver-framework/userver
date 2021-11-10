#pragma once

#include <memory>

#include <grpcpp/completion_queue.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::grpc::impl {

template <typename Handler>
class ReactorData final {
 public:
  using Service = typename Handler::Service;
  using Stub = typename Service::AsyncService;

  ReactorData(std::unique_ptr<Handler> handler,
              ::grpc::ServerCompletionQueue& queue,
              engine::TaskProcessor& handler_task_processor)
      : handler_(std::move(handler)),
        queue_(queue),
        handler_task_processor_(handler_task_processor) {
    UASSERT(handler_);
  }

  ~ReactorData() { wait_tokens_.WaitForAllTokens(); }

  Handler& GetHandler() { return *handler_; }

  Stub& GetStub() { return stub_; }

  ::grpc::ServerCompletionQueue& GetQueue() { return queue_; }

  utils::impl::WaitTokenStorage::Token GetWaitToken() {
    return wait_tokens_.GetToken();
  }

  engine::TaskProcessor& GetHandlerTaskProcessor() {
    return handler_task_processor_;
  }

 private:
  std::unique_ptr<Handler> handler_;
  ::grpc::ServerCompletionQueue& queue_;
  engine::TaskProcessor& handler_task_processor_;
  Stub stub_;
  utils::impl::WaitTokenStorage wait_tokens_;
};

}  // namespace server::grpc::impl

USERVER_NAMESPACE_END
