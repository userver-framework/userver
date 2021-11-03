#pragma once

#include <optional>
#include <vector>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>

#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>

#include <userver/clients/grpc/channels.hpp>
#include <userver/clients/grpc/queue_holder.hpp>

USERVER_NAMESPACE_BEGIN

template <typename GrpcServiceImpl>
class GrpcServiceFixture : public ::testing::Test {
 protected:
  GrpcServiceFixture() {
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("localhost:0", ::grpc::InsecureServerCredentials(),
                             &server_port_);
    builder.RegisterService(&service_);
    server_ = builder.BuildAndStart();
    LOG_INFO() << "Test fixture gRPC server started on port " << server_port_;

    queue_holder_.emplace();
    endpoint_ = "localhost:" + std::to_string(server_port_);
    channel_ = clients::grpc::MakeChannel(
        engine::current_task::GetTaskProcessor(),
        ::grpc::InsecureChannelCredentials(), endpoint_);
  }

  ~GrpcServiceFixture() {
    // Must shutdown server, then queues before anything else
    server_->Shutdown();
    queue_holder_.reset();
  }

  std::shared_ptr<::grpc::Channel> GetChannel() { return channel_; }

  ::grpc::CompletionQueue& GetQueue() { return queue_holder_->GetQueue(); }

 private:
  int server_port_ = 0;
  GrpcServiceImpl service_;
  std::unique_ptr<::grpc::Server> server_;
  std::optional<clients::grpc::QueueHolder> queue_holder_;
  std::string endpoint_;
  std::shared_ptr<::grpc::Channel> channel_;
};

USERVER_NAMESPACE_END
