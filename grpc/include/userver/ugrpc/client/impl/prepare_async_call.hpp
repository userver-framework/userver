#pragma once

#include <functional>
#include <utility>

#include <grpcpp/support/async_stream.h>

#include <userver/ugrpc/impl/stub_any.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

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

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
