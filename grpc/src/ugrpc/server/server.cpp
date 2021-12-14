#include <userver/ugrpc/server/server.hpp>

#include <exception>
#include <optional>
#include <vector>

#include <fmt/format.h>
#include <grpcpp/server.h>

#include <userver/engine/mutex.hpp>
#include <userver/logging/log.hpp>

#include <userver/ugrpc/server/queue_holder.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class Server::Impl final {
 public:
  Impl();
  ~Impl();

  void AddListeningPort(int port);

  void AddReactor(std::unique_ptr<Reactor>&& reactor);

  ::grpc::ServerBuilder& GetServerBuilder() noexcept;

  ::grpc::ServerCompletionQueue& GetCompletionQueue() noexcept;

  void Start();

  int GetPort() const noexcept;

  void Stop() noexcept;

 private:
  enum class State {
    kConfiguration,
    kActive,
    kStopped,
  };

  void DoStart();

  State state_{State::kConfiguration};
  ::grpc::ServerBuilder server_builder_;
  std::optional<int> port_;
  std::vector<std::unique_ptr<ugrpc::server::Reactor>> reactors_;
  std::optional<ugrpc::server::QueueHolder> queue_;
  std::unique_ptr<::grpc::Server> server_;
  engine::Mutex configuration_mutex_;
};

Server::Impl::Impl() {
  LOG_INFO() << "Configuring the gRPC server";
  queue_.emplace(server_builder_);
}

Server::Impl::~Impl() {
  UASSERT_MSG(
      state_ != State::kActive,
      "The user must explicitly call 'Stop' after successfully starting the "
      "gRPC server to ensure that it is destroyed before handlers.");
}

void Server::Impl::AddListeningPort(int port) {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  UASSERT_MSG(!port_,
              "As of now, AddListeningPort can be called no more than once");
  port_ = port;
  UINVARIANT(port >= 0 && port <= 65535, "Invalid gRPC listening port");

  const auto uri = fmt::format("[::1]:{}", port);
  server_builder_.AddListeningPort(uri, ::grpc::InsecureServerCredentials(),
                                   &*port_);
}

void Server::Impl::AddReactor(std::unique_ptr<Reactor>&& reactor) {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  reactors_.push_back(std::move(reactor));
}

::grpc::ServerBuilder& Server::Impl::GetServerBuilder() noexcept {
  UASSERT(state_ == State::kConfiguration);
  return server_builder_;
}

::grpc::ServerCompletionQueue& Server::Impl::GetCompletionQueue() noexcept {
  UASSERT(state_ == State::kConfiguration || state_ == State::kActive);
  return queue_->GetQueue();
}

void Server::Impl::Start() {
  std::lock_guard lock(configuration_mutex_);
  UASSERT(state_ == State::kConfiguration);

  try {
    DoStart();
    state_ = State::kActive;
  } catch (const std::exception& ex) {
    LOG_ERROR() << "The gRPC server failed to start. " << ex;
    Stop();
    throw;
  }
}

int Server::Impl::GetPort() const noexcept {
  UASSERT(state_ == State::kActive);
  UASSERT_MSG(port_, "No port has been registered using AddListeningPort");
  return *port_;
}

void Server::Impl::Stop() noexcept {
  // Note 1: Stop must be idempotent, so that the 'Stop' invocation after a
  // 'Start' failure is optional.
  // Note 2: 'state_' remains 'kActive' while stopping, which allows clients to
  // finish their requests using 'queue_'.

  // Must shutdown server, then queues, then reactors before anything else
  if (server_) {
    LOG_INFO() << "Stopping the gRPC server";
    server_->Shutdown();
  }
  queue_.reset();
  reactors_.clear();
  server_.reset();

  state_ = State::kStopped;
}

void Server::Impl::DoStart() {
  LOG_INFO() << "Starting the gRPC server";

  for (auto& reactor : reactors_) {
    server_builder_.RegisterService(&reactor->GetService());
  }

  server_ = server_builder_.BuildAndStart();
  UINVARIANT(server_, "See grpcpp logs for details");

  for (auto& reactor : reactors_) {
    reactor->Start();
  }

  if (port_) {
    LOG_INFO() << "gRPC server started on port " << *port_;
  } else {
    LOG_INFO() << "gRPC server started without using AddListeningPort";
  }
}

Server::Server() : impl_() {}

Server::~Server() = default;

void Server::AddListeningPort(int port) { impl_->AddListeningPort(port); }

::grpc::ServerBuilder& Server::GetServerBuilder() noexcept {
  return impl_->GetServerBuilder();
}

::grpc::CompletionQueue& Server::GetCompletionQueue() noexcept {
  return impl_->GetCompletionQueue();
}

void Server::Start() { return impl_->Start(); }

int Server::GetPort() const noexcept { return impl_->GetPort(); }

void Server::Stop() noexcept { return impl_->Stop(); }

::grpc::ServerCompletionQueue& Server::GetServerCompletionQueue() noexcept {
  return impl_->GetCompletionQueue();
}

void Server::AddReactor(std::unique_ptr<Reactor>&& reactor) {
  impl_->AddReactor(std::move(reactor));
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
