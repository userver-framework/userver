#include <userver/clients/grpc/manager.hpp>

#include <thread>

#include <grpcpp/grpcpp.h>

#include <userver/engine/async.hpp>

#include <userver/clients/grpc/impl/async_methods.hpp>

namespace clients::grpc {

namespace {

class QueueRunner final {
 public:
  QueueRunner() {
    worker_thread_ = std::thread([this] { ProcessQueue(); });
  }

  ~QueueRunner() {
    cq_.Shutdown();
    if (worker_thread_.joinable()) {
      worker_thread_.join();
    }
  }

  QueueRunner(const QueueRunner&) = delete;

  ::grpc::CompletionQueue& GetQueue() { return cq_; }

 private:
  void ProcessQueue() noexcept {
    void* tag = nullptr;
    bool ok = false;

    while (cq_.Next(&tag, &ok)) {
      auto* call = static_cast<impl::AsyncMethodInvocation*>(tag);
      UASSERT(call != nullptr);
      call->Notify(ok);
    }
  }

 private:
  ::grpc::CompletionQueue cq_;
  std::thread worker_thread_;
};

}  // namespace

struct Manager::Impl final {
  explicit Impl(engine::TaskProcessor& processor) : task_processor(processor) {}

  engine::TaskProcessor& task_processor;
  QueueRunner queue_runner;
};

Manager::Manager(engine::TaskProcessor& processor) : impl_(processor) {}

Manager::~Manager() = default;

Manager::ChannelPtr Manager::GetChannel(const std::string& endpoint) {
  // Spawn a blocking task creating a gRPC channel
  // This is third party code, no use of span inside it
  auto task = engine::impl::Async(impl_->task_processor, [endpoint] {
    return ::grpc::CreateChannel(endpoint,
                                 ::grpc::InsecureChannelCredentials());
  });
  return task.Get();
}

::grpc::CompletionQueue& Manager::GetCompletionQueue() {
  return impl_->queue_runner.GetQueue();
}

}  // namespace clients::grpc
