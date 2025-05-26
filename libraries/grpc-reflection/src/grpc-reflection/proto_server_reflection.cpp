/*
 *
 * Copyright 2016 gRPC authors.
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
/* based on
 * https://github.com/grpc/grpc/blob/0f9d024fec6a96cfa07ebae633e3ee96c933d3c4/src/cpp/ext/proto_server_reflection.cc
 */

#include <grpc-reflection/proto_server_reflection.hpp>

#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <grpcpp/support/interceptor.h>

// IWYU pragma: no_include <google/protobuf/descriptor.h>

namespace proto_reflection = grpc::reflection::v1alpha;

USERVER_NAMESPACE_BEGIN

namespace grpc_reflection {

ProtoServerReflection::ProtoServerReflection() : descriptor_pool_(grpc::protobuf::DescriptorPool::generated_pool()) {}

void ProtoServerReflection::AddService(std::string_view service) {
    auto ptr = services_.Lock();
    ptr->insert(std::move(service));
}

void ProtoServerReflection::AddServiceList(const std::vector<std::string_view>& services) {
    auto ptr = services_.Lock();
    for (auto&& service : services) {
        ptr->insert(std::move(service));
    }
}

ProtoServerReflection::ServerReflectionInfoResult
ProtoServerReflection::ServerReflectionInfo(CallContext& /*context*/, ServerReflectionInfoReaderWriter& stream) {
    proto_reflection::ServerReflectionRequest request;
    proto_reflection::ServerReflectionResponse response;
    ::grpc::Status status;
    while (stream.Read(request)) {
        switch (request.message_request_case()) {
            case proto_reflection::ServerReflectionRequest::MessageRequestCase::kFileByFilename:
                status = GetFileByName(request.file_by_filename(), response);
                break;
            case proto_reflection::ServerReflectionRequest::MessageRequestCase::kFileContainingSymbol:
                status = GetFileContainingSymbol(request.file_containing_symbol(), response);
                break;
            case proto_reflection::ServerReflectionRequest::MessageRequestCase::kFileContainingExtension:
                status = GetFileContainingExtension(request.file_containing_extension(), response);
                break;
            case proto_reflection::ServerReflectionRequest::MessageRequestCase::kAllExtensionNumbersOfType:
                status = GetAllExtensionNumbers(
                    request.all_extension_numbers_of_type(), *response.mutable_all_extension_numbers_response()
                );
                break;
            case proto_reflection::ServerReflectionRequest::MessageRequestCase::kListServices:
                status = ListService(*response.mutable_list_services_response());
                break;
            default:
                status = ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
        }

        if (!status.ok()) {
            FillErrorResponse(status, *response.mutable_error_response());
        }
        response.set_valid_host(request.host());
        response.set_allocated_original_request(new proto_reflection::ServerReflectionRequest(std::move(request)));
        stream.Write(response);
    }

    return grpc::Status::OK;
}

void ProtoServerReflection::FillErrorResponse(
    const ::grpc::Status& status,
    proto_reflection::ErrorResponse& error_response
) {
    error_response.set_error_code(status.error_code());
    error_response.set_error_message(status.error_message());
}

::grpc::Status ProtoServerReflection::ListService(proto_reflection::ListServiceResponse& response) {
    auto ptr = services_.Lock();
    if (ptr->empty()) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Services not found.");
    }
    for (const auto& value : *ptr) {
        proto_reflection::ServiceResponse* service_response = response.add_service();
        *service_response->mutable_name() = value;
    }
    return ::grpc::Status::OK;
}

::grpc::Status
ProtoServerReflection::GetFileByName(std::string_view file_name, proto_reflection::ServerReflectionResponse& response) {
    if (descriptor_pool_ == nullptr) {
        return ::grpc::Status::CANCELLED;
    }

    const grpc::protobuf::FileDescriptor* file_desc =
#if GOOGLE_PROTOBUF_VERSION >= 4022000
        descriptor_pool_->FindFileByName(file_name);
#else
        descriptor_pool_->FindFileByName(std::string{file_name});
#endif
    if (file_desc == nullptr) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "File not found.");
    }
    std::unordered_set<std::string_view> seen_files;
    FillFileDescriptorResponse(*file_desc, response, seen_files);
    return ::grpc::Status::OK;
}

::grpc::Status ProtoServerReflection::GetFileContainingSymbol(
    std::string_view symbol,
    proto_reflection::ServerReflectionResponse& response
) {
    if (descriptor_pool_ == nullptr) {
        return ::grpc::Status::CANCELLED;
    }

#if GOOGLE_PROTOBUF_VERSION >= 4022000
    const grpc::protobuf::FileDescriptor* file_desc = descriptor_pool_->FindFileContainingSymbol(symbol);
#else
    const grpc::protobuf::FileDescriptor* file_desc = descriptor_pool_->FindFileContainingSymbol(std::string{symbol});
#endif
    if (file_desc == nullptr) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Symbol not found.");
    }
    std::unordered_set<std::string_view> seen_files;
    FillFileDescriptorResponse(*file_desc, response, seen_files);
    return ::grpc::Status::OK;
}

::grpc::Status ProtoServerReflection::GetFileContainingExtension(
    const proto_reflection::ExtensionRequest& request,
    proto_reflection::ServerReflectionResponse& response
) {
    if (descriptor_pool_ == nullptr) {
        return ::grpc::Status::CANCELLED;
    }

    const grpc::protobuf::Descriptor* desc = descriptor_pool_->FindMessageTypeByName(request.containing_type());
    if (desc == nullptr) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Type not found.");
    }

    const grpc::protobuf::FieldDescriptor* field_desc =
        descriptor_pool_->FindExtensionByNumber(desc, request.extension_number());
    if (field_desc == nullptr) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Extension not found.");
    }
    std::unordered_set<std::string_view> seen_files;
    FillFileDescriptorResponse(*(field_desc->file()), response, seen_files);
    return ::grpc::Status::OK;
}

::grpc::Status ProtoServerReflection::GetAllExtensionNumbers(
    std::string_view type,
    proto_reflection::ExtensionNumberResponse& response
) {
    if (descriptor_pool_ == nullptr) {
        return ::grpc::Status::CANCELLED;
    }

#if GOOGLE_PROTOBUF_VERSION >= 4022000
    const grpc::protobuf::Descriptor* desc = descriptor_pool_->FindMessageTypeByName(type);
#else
    const grpc::protobuf::Descriptor* desc = descriptor_pool_->FindMessageTypeByName(std::string{type});
#endif
    if (desc == nullptr) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Type not found.");
    }

    std::vector<const grpc::protobuf::FieldDescriptor*> extensions;
    descriptor_pool_->FindAllExtensions(desc, &extensions);
    for (const auto& value : extensions) {
        response.add_extension_number(value->number());
    }
    *response.mutable_base_type_name() = type;
    return ::grpc::Status::OK;
}

void ProtoServerReflection::FillFileDescriptorResponse(
    const grpc::protobuf::FileDescriptor& file_desc,
    proto_reflection::ServerReflectionResponse& response,
    std::unordered_set<std::string_view>& seen_files
) {
    if (seen_files.find(file_desc.name()) != seen_files.end()) {
        return;
    }
    seen_files.insert(file_desc.name());

    grpc::protobuf::FileDescriptorProto file_desc_proto;
    grpc::string data;
    file_desc.CopyTo(&file_desc_proto);
    if (!file_desc_proto.SerializeToString(&data)) {
        throw std::runtime_error("Failed to serialize file_desc_proto to string");
    }
    response.mutable_file_descriptor_response()->add_file_descriptor_proto(data);

    for (int i = 0; i < file_desc.dependency_count(); ++i) {
        FillFileDescriptorResponse(*file_desc.dependency(i), response, seen_files);
    }
}

}  // namespace grpc_reflection

USERVER_NAMESPACE_END
