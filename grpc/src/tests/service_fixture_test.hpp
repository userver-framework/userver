#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/server_builder.h>

#include <userver/engine/task/task.hpp>
#include <userver/utest/utest.hpp>

#include <userver/ugrpc/server/queue_holder.hpp>
#include <userver/ugrpc/server/reactor.hpp>

USERVER_NAMESPACE_BEGIN

// Sets up a mini gRPC server using the provided handlers
class GrpcServiceFixture : public ::testing::Test {
 protected:
  GrpcServiceFixture();
  ~GrpcServiceFixture() override;

  template <typename Handler>
  void RegisterHandler(std::unique_ptr<Handler> handler) {
    reactors_.push_back(
        Handler::MakeReactor(std::move(handler), queue_holder_->GetQueue(),
                             engine::current_task::GetTaskProcessor()));
  }

  // Must be called after the handlers are unregistered
  void StartServer();

  // Must be called in the destructor of the derived fixture
  void StopServer() noexcept;

  std::shared_ptr<::grpc::Channel> GetChannel();

  ::grpc::CompletionQueue& GetQueue();

 private:
  ::grpc::ServerBuilder builder_;
  int server_port_ = 0;
  std::vector<std::unique_ptr<ugrpc::server::Reactor>> reactors_;
  std::optional<ugrpc::server::QueueHolder> queue_holder_;
  std::unique_ptr<::grpc::Server> server_;
  std::shared_ptr<::grpc::Channel> channel_;
};

// Sets up a mini gRPC server using a single default-constructed handler
template <typename Handler>
class GrpcServiceFixtureSimple : public GrpcServiceFixture {
 protected:
  GrpcServiceFixtureSimple() {
    RegisterHandler(std::make_unique<Handler>());
    StartServer();
  }

  ~GrpcServiceFixtureSimple() { StopServer(); }
};

USERVER_NAMESPACE_END
