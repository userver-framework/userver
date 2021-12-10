#include <tests/service_fixture_test.hpp>

#include <fmt/format.h>
#include <grpcpp/security/server_credentials.h>

#include <userver/logging/log.hpp>

#include <userver/ugrpc/client/channels.hpp>

USERVER_NAMESPACE_BEGIN

GrpcServiceFixture::GrpcServiceFixture() {
  builder_.AddListeningPort("localhost:0", ::grpc::InsecureServerCredentials(),
                            &server_port_);
  queue_holder_.emplace(builder_);
}

GrpcServiceFixture::~GrpcServiceFixture() {
  UASSERT_MSG(!server_,
              "Forgot to call StopServer in the fixture's destructor");
}

void GrpcServiceFixture::StartServer() {
  UASSERT(!server_);

  for (auto& reactor : reactors_) {
    builder_.RegisterService(&reactor->GetService());
  }

  server_ = builder_.BuildAndStart();
  UASSERT(server_);

  for (auto& reactor : reactors_) {
    reactor->Start();
  }

  LOG_INFO() << "Test fixture gRPC server started on port " << server_port_;
  channel_ =
      ugrpc::client::MakeChannel(engine::current_task::GetTaskProcessor(),
                                 ::grpc::InsecureChannelCredentials(),
                                 fmt::format("localhost:{}", server_port_));
}

void GrpcServiceFixture::StopServer() noexcept {
  // Must shutdown server, then queues before anything else
  if (server_) {
    server_->Shutdown();
  }
  queue_holder_.reset();
  reactors_.clear();
  server_.reset();
}

std::shared_ptr<::grpc::Channel> GrpcServiceFixture::GetChannel() {
  UASSERT(channel_);
  return channel_;
}

::grpc::CompletionQueue& GrpcServiceFixture::GetQueue() {
  UASSERT(queue_holder_);
  return queue_holder_->GetQueue();
}

USERVER_NAMESPACE_END
