#pragma once

#include <memory>

#include <grpcpp/channel.h>

#include <userver/engine/task/task.hpp>
#include <userver/utest/utest.hpp>

#include <userver/ugrpc/server/server.hpp>

USERVER_NAMESPACE_BEGIN

// Sets up a mini gRPC server using the provided handlers
class GrpcServiceFixture : public ::testing::Test {
 protected:
  GrpcServiceFixture();
  ~GrpcServiceFixture() override;

  template <typename Handler>
  void RegisterHandler(std::unique_ptr<Handler>&& handler) {
    server_.AddHandler(std::move(handler),
                       engine::current_task::GetTaskProcessor());
  }

  // Must be called after the handlers are unregistered
  void StartServer();

  // Must be called in the destructor of the derived fixture
  void StopServer() noexcept;

  std::shared_ptr<::grpc::Channel> GetChannel();

  ::grpc::CompletionQueue& GetQueue();

 private:
  ugrpc::server::Server server_;
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
