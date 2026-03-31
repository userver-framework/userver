#pragma once

#include <functional>
#include <utility>

#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>

#include <userver/ugrpc/impl/stub_any.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <class Stub, class Request, class Response>
using PrepareUnaryCall = std::unique_ptr<grpc::ClientAsyncResponseReader<
    Response>> (Stub::*)(::grpc::ClientContext*, const Request&, ::grpc::CompletionQueue*);

template <class Stub, class Request, class Response>
using PrepareServerStreamingCall = std::unique_ptr<
    grpc::ClientAsyncReader<Response>> (Stub::*)(::grpc::ClientContext*, const Request&, ::grpc::CompletionQueue*);

template <class Stub, class Request, class Response>
using PrepareClientStreamingCall = std::unique_ptr<
    grpc::ClientAsyncWriter<Request>> (Stub::*)(::grpc::ClientContext*, Response*, ::grpc::CompletionQueue*);

template <class Stub, class Request, class Response>
using PrepareBidiStreamingCall = std::unique_ptr<
    grpc::ClientAsyncReaderWriter<Request, Response>> (Stub::*)(::grpc::ClientContext*, ::grpc::CompletionQueue*);

template <typename F, class Stub, typename... Args>
decltype(auto) PrepareAsyncCall(F Stub::*prepare_async_method, ugrpc::impl::StubAny& stub, Args&&... args) {
    return std::invoke(prepare_async_method, ugrpc::impl::StubCast<Stub>(stub), std::forward<Args>(args)...);
}

template <class Stub, class Request, class Response>
class PrepareUnaryCallProxy {
public:
    explicit PrepareUnaryCallProxy(PrepareUnaryCall<Stub, Request, Response> prepare_async_method)
        : prepare_async_method_{prepare_async_method}
    {}

    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const {
        return impl::PrepareAsyncCall(prepare_async_method_, std::forward<Args>(args)...);
    }

private:
    PrepareUnaryCall<Stub, Request, Response> prepare_async_method_;
};

template <typename Stub, class Request, class Response>
PrepareUnaryCallProxy(PrepareUnaryCall<Stub, Request, Response>) -> PrepareUnaryCallProxy<Stub, Request, Response>;

using GenericPrepareUnaryCall = std::unique_ptr<grpc::ClientAsyncResponseReader<
    grpc::ByteBuffer>> (grpc::GenericStub::*)(grpc::ClientContext*, const grpc::string&, const grpc::ByteBuffer&, grpc::CompletionQueue*);

template <>
class PrepareUnaryCallProxy<grpc::GenericStub, grpc::ByteBuffer, grpc::ByteBuffer> {
public:
    PrepareUnaryCallProxy(GenericPrepareUnaryCall prepare_async_method, grpc::string method_name)
        : prepare_async_method_{prepare_async_method},
          method_name_{std::move(method_name)}
    {}

    decltype(auto) operator()(
        ugrpc::impl::StubAny& stub,
        grpc::ClientContext* client_context,
        const grpc::ByteBuffer& request,
        grpc::CompletionQueue* cq
    ) const {
        return impl::PrepareAsyncCall(prepare_async_method_, stub, client_context, method_name_, request, cq);
    }

private:
    GenericPrepareUnaryCall prepare_async_method_;
    grpc::string method_name_;
};

PrepareUnaryCallProxy(GenericPrepareUnaryCall, const grpc::string&)
    -> PrepareUnaryCallProxy<grpc::GenericStub, grpc::ByteBuffer, grpc::ByteBuffer>;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
