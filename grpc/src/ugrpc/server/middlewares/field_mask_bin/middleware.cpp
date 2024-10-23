#include "middleware.hpp"

#include <string_view>
#include <utility>

#include <google/protobuf/field_mask.pb.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/util/field_mask_util.h>
#include <grpcpp/support/status.h>

#include <userver/logging/log.hpp>
#include <userver/ugrpc/field_mask.hpp>
#include <userver/ugrpc/server/metadata_utils.hpp>
#include <userver/ugrpc/server/middlewares/field_mask_bin/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::field_mask_bin {

namespace {

template <typename T>
FieldMask ConstructFieldMask(T&& metadata) {
    google::protobuf::FieldMask google_field_mask;
    for (const std::string_view field_mask_str : metadata) {
        google::protobuf::FieldMask one_field_mask;

#if GOOGLE_PROTOBUF_VERSION >= 3022000
        bool success = one_field_mask.ParseFromString(field_mask_str);
#else
        bool success = one_field_mask.ParseFromString(std::string(field_mask_str));
#endif
        if (success) {
            google::protobuf::util::FieldMaskUtil::Union(google_field_mask, one_field_mask, &google_field_mask);
        } else {
            LOG_WARNING() << "Failed to construct field mask. Falling back to empty.";
        }
    }
    return FieldMask(google_field_mask);
}

}  // namespace

Middleware::Middleware(std::string metadata_field_name) : metadata_field_name_(std::move(metadata_field_name)) {}

void Middleware::Handle(ugrpc::server::MiddlewareCallContext& context) const {
    try {
        context.GetCall().GetStorageContext().Set(
            kFieldMaskStorageDataTag, ConstructFieldMask(GetRepeatedMetadata(context.GetCall(), metadata_field_name_))
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
        LOG_WARNING() << "Failed to trim the response " << e.what();
        context.GetCall().FinishWithError(::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, e.what()));
    }
}

}  // namespace ugrpc::server::middlewares::field_mask_bin

USERVER_NAMESPACE_END
