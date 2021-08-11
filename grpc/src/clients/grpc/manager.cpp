#include <userver/clients/grpc/manager.hpp>

#include <thread>

#include <grpcpp/grpcpp.h>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>

#include <userver/clients/grpc/detail/async_invocation.hpp>

namespace clients::grpc {

class QueueRunner {
 public:
  QueueRunner() { Run(); }
  ~QueueRunner() { Shutdown(); }

  QueueRunner(const QueueRunner&) = delete;

  ::grpc::CompletionQueue& GetQueue() { return cq_; }

 private:
  void Run() {
    worker_thread_ = std::make_unique<std::thread>([&] { ProcessQueue(); });
  }
  void Shutdown() {
    cq_.Shutdown();
    if (worker_thread_) {
      worker_thread_->join();
      worker_thread_.reset();
    }
  }

  void ProcessQueue();

 private:
  ::grpc::CompletionQueue cq_;
  std::unique_ptr<std::thread> worker_thread_;
};

struct Manager::Impl {
  Impl(engine::TaskProcessor& processor) : task_processor_{processor} {}

  ChannelPtr GetChannel(const std::string& endpoint) {
    // Spawn a blocking task creating a gRPC channel
    // This is third party code, no use of span inside it
    auto task = engine::impl::Async(
        task_processor_,
        [](const std::string& endpoint) {
          return ::grpc::CreateChannel(endpoint,
                                       ::grpc::InsecureChannelCredentials());
        },
        endpoint);
    return task.Get();
  }

  ::grpc::CompletionQueue& GetQueue() { return queue_runner_.GetQueue(); }

  engine::TaskProcessor& task_processor_;
  QueueRunner queue_runner_;
};

Manager::Manager(engine::TaskProcessor& processor)
    : pimpl_{std::make_unique<Impl>(processor)} {}

Manager::~Manager() = default;

Manager::ChannelPtr Manager::GetChannel(const std::string& endpoint) {
  return pimpl_->GetChannel(endpoint);
}

::grpc::CompletionQueue& Manager::GetCompletionQueue() {
  return pimpl_->GetQueue();
}

void QueueRunner::ProcessQueue() {
  void* tag = nullptr;
  bool finished_ok = false;

  while (cq_.Next(&tag, &finished_ok)) {
    // TODO Check the ok flag.
    // It corresponds to the success or failure of Finish operation
    if (tag) {
      try {
        auto* invocation = static_cast<detail::AsyncInvocationBase*>(tag);
        invocation->ProcessResult(finished_ok);
      } catch (const std::exception& e) {
        LOG_ERROR() << "Error processing queue: " << e.what();
      }
    }
  }
}

}  // namespace clients::grpc
