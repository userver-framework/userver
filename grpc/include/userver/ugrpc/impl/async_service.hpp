#pragma once

#include <grpcpp/channel.h>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/impl/service_type.h>

#include <userver/ugrpc/impl/stub_any.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

template <typename Service>
class AsyncService final : public Service::AsyncService {
public:
    using grpc::Service::RequestAsyncBidiStreaming;
    using grpc::Service::RequestAsyncClientStreaming;
    using grpc::Service::RequestAsyncServerStreaming;
    using grpc::Service::RequestAsyncUnary;

    static StubAny NewStub(const std::shared_ptr<grpc::Channel>& channel) {
        return MakeStub<typename Service::Stub>(channel);
    }
};

template <>
class AsyncService<grpc::AsyncGenericService> final {
public:
    grpc::AsyncGenericService& GetAsyncGenericService() { return async_generic_service_; }

    static StubAny NewStub(std::shared_ptr<grpc::Channel> channel) {
        return MakeStub<grpc::GenericStub>(std::move(channel));
    }

private:
    grpc::AsyncGenericService async_generic_service_;
};

using AsyncGenericService = AsyncService<grpc::AsyncGenericService>;

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
