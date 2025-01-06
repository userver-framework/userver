/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/* Code was modified by Yandex developers
 * based on
 * https://github.com/grpc/grpc/blob/0f9d024fec6a96cfa07ebae633e3ee96c933d3c4/src/cpp/ext/proto_server_reflection.cc
 */

#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/config_protobuf.h>
#include <grpcpp/support/config.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/sync_stream.h>

#include <reflection.grpc.pb.h>
#include <reflection.pb.h>
#include <reflection_service.usrv.pb.hpp>

#include <userver/concurrent/variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_reflection {

class ProtoServerReflection final : public grpc::reflection::v1alpha::ServerReflectionBase {
public:
    ProtoServerReflection();

    // Add the full names of registered services
    void AddService(std::string_view service);

    // Add the full names of registered services
    void AddServiceList(const std::vector<std::string_view>& services);

    // implementation of ServerReflectionInfo(stream ServerReflectionRequest) rpc
    // in ServerReflection service
    ServerReflectionInfoResult ServerReflectionInfo(CallContext& context, ServerReflectionInfoReaderWriter& stream)
        override;

private:
    grpc::Status ListService(grpc::reflection::v1alpha::ListServiceResponse& response);

    grpc::Status
    GetFileByName(std::string_view file_name, grpc::reflection::v1alpha::ServerReflectionResponse& response);

    grpc::Status
    GetFileContainingSymbol(std::string_view symbol, grpc::reflection::v1alpha::ServerReflectionResponse& response);

    grpc::Status GetFileContainingExtension(
        const grpc::reflection::v1alpha::ExtensionRequest& request,
        grpc::reflection::v1alpha::ServerReflectionResponse& response
    );

    grpc::Status
    GetAllExtensionNumbers(std::string_view type, grpc::reflection::v1alpha::ExtensionNumberResponse& response);

    void FillFileDescriptorResponse(
        const grpc::protobuf::FileDescriptor& file_desc,
        grpc::reflection::v1alpha::ServerReflectionResponse& response,
        std::unordered_set<std::string_view>& seen_files
    );

    void FillErrorResponse(const grpc::Status& status, grpc::reflection::v1alpha::ErrorResponse& error_response);

    const grpc::protobuf::DescriptorPool* descriptor_pool_;
    concurrent::Variable<std::unordered_set<std::string_view>> services_;
};

}  // namespace grpc_reflection

USERVER_NAMESPACE_END
