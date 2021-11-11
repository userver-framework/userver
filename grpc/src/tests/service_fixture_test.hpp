#pragma once

#include <optional>
#include <vector>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>

#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>

#include <userver/ugrpc/client/channels.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>
#include <userver/ugrpc/server/queue_holder.hpp>
#include <userver/ugrpc/server/reactor.hpp>

USERVER_NAMESPACE_BEGIN

template <typename Handler>
class GrpcServiceFixture : public ::testing::Test {
 protected:
  GrpcServiceFixture() {
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort("localhost:0", ::grpc::InsecureServerCredentials(),
                             &server_port_);
    queue_holder_.emplace(builder);
    reactor_ = Handler::MakeReactor(std::make_unique<Handler>(),
                                    queue_holder_->GetQueue(),
                                    engine::current_task::GetTaskProcessor());
    builder.RegisterService(&reactor_->GetService());
    server_ = builder.BuildAndStart();

    reactor_->Start();
    LOG_INFO() << "Test fixture gRPC server started on port " << server_port_;
    endpoint_ = "localhost:" + std::to_string(server_port_);
    channel_ = ugrpc::client::MakeChannel(
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
  std::unique_ptr<ugrpc::server::Reactor> reactor_;
  std::optional<ugrpc::server::QueueHolder> queue_holder_;
  std::unique_ptr<::grpc::Server> server_;
  std::string endpoint_;
  std::shared_ptr<::grpc::Channel> channel_;
};

USERVER_NAMESPACE_END
