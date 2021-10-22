#pragma once

#include <thread>

#include <grpcpp/grpcpp.h>

#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>

#include <userver/clients/grpc/manager.hpp>
#include <userver/clients/grpc/service.hpp>

USERVER_NAMESPACE_BEGIN

template <typename GrpcService, typename GrpcServiceImpl>
class GrpcServiceFixture : public ::testing::Test {
 protected:
  GrpcServiceFixture() {
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("localhost:0", ::grpc::InsecureServerCredentials(),
                             &server_port_);
    builder.RegisterService(&service_);
    server_ = builder.BuildAndStart();
    LOG_INFO() << "Test fixture gRPC server started on port " << server_port_;
    server_thread_ = std::thread([&] { server_->Wait(); });
  }

  ~GrpcServiceFixture() {
    if (server_) {
      server_->Shutdown();
    }
    server_thread_.join();
  }

  auto ClientChannel() { return manager_.GetChannel(ServerEndpoint()); }

  auto ClientStub() const { return GrpcService::NewStub(ClientChannel()); }

  auto& GetQueue() { return manager_.GetCompletionQueue(); }

 private:
  std::string ServerEndpoint() const {
    return "localhost:" + std::to_string(server_port_);
  }

 private:
  int server_port_ = 0;
  GrpcServiceImpl service_;
  std::unique_ptr<::grpc::Server> server_;
  std::thread server_thread_;
  clients::grpc::Manager manager_{engine::current_task::GetTaskProcessor()};
};

USERVER_NAMESPACE_END
