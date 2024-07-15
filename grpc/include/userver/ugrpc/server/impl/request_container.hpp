#pragma once

#include <grpcpp/server_context.h>

#include <userver/utils/algo.hpp>

#include <userver/server/request/task_inherited_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

class GrpcRequestContainer final
    : public USERVER_NAMESPACE::server::request::RequestContainer {
 public:
  GrpcRequestContainer(const grpc::ServerContext& server_context)
      : server_context_(server_context){};
  std::string_view GetHeader(std::string_view header_name) const override {
    const auto& header_value = utils::FindOrDefault(
        server_context_.client_metadata(),
        grpc::string_ref{header_name.data(), header_name.size()});
    return {header_value.data(), header_value.size()};
  };
  bool HasHeader(std::string_view header_name) const override {
    return nullptr !=
           utils::FindOrNullptr(
               server_context_.client_metadata(),
               grpc::string_ref{header_name.data(), header_name.size()});
  };

 private:
  const grpc::ServerContext& server_context_;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
