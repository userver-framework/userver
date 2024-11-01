#include "middleware.hpp"

#include <string_view>

#include <google/protobuf/field_mask.pb.h>
#include <google/protobuf/util/field_mask_util.h>
#include <grpcpp/support/status.h>

#include <userver/logging/log.hpp>
#include <userver/ugrpc/field_mask.hpp>
#include <userver/ugrpc/server/metadata_utils.hpp>
#include <userver/ugrpc/server/middlewares/field_mask/component.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::field_mask {

namespace {

FieldMask ConstructFieldMask(ugrpc::server::CallAnyBase& call, std::string_view field_name) {
    const auto& metadata = GetRepeatedMetadata(call, field_name);
    if (metadata.empty()) return FieldMask();
    return FieldMask(*metadata.begin(), FieldMask::Encoding::kWebSafeBase64);
}

}  // namespace

Middleware::Middleware(std::string metadata_field_name)
    : metadata_field_name_(utils::text::ToLower(metadata_field_name)) {}

void Middleware::Handle(ugrpc::server::MiddlewareCallContext& context) const {
    try {
        context.GetCall().GetStorageContext().Set(
            kFieldMaskStorageDataTag, ConstructFieldMask(context.GetCall(), metadata_field_name_)
        );
        context.Next();
    } catch (const FieldMask::BadPathError& e) {
        LOG_WARNING() << "Failed to construct the field mask: " << e.what();
        context.GetCall().FinishWithError(::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, e.what()));
    }
}

void Middleware::CallResponseHook(
    const ugrpc::server::MiddlewareCallContext& context,
    google::protobuf::Message& response
) {
    const FieldMask& field_mask = context.GetCall().GetStorageContext().Get(kFieldMaskStorageDataTag);
    try {
        field_mask.Trim(response);
    } catch (const FieldMask::BadPathError& e) {
        LOG_WARNING() << "Failed to trim the response: " << e.what();
        context.GetCall().FinishWithError(::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, e.what()));
    }
}

}  // namespace ugrpc::server::middlewares::field_mask

USERVER_NAMESPACE_END
