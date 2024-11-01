#pragma once

/// @file userver/ugrpc/server/middlewares/field_mask/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::field_mask::Component

#include <userver/ugrpc/server/middlewares/base.hpp>

#include <memory>
#include <string>
#include <string_view>

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>
#include <userver/ugrpc/field_mask.hpp>
#include <userver/ugrpc/server/storage_context.hpp>
#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

/// Server field-mask metadata field middleware
namespace ugrpc::server::middlewares::field_mask {

/// @see ugrpc::server::middlewares::field_mask::Component
inline const utils::AnyStorageDataTag<ugrpc::server::StorageContext, FieldMask> kFieldMaskStorageDataTag;

/// @see ugrpc::server::middlewares::field_mask::Component
inline const std::string kDefaultMetadataFieldName = "field-mask";

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server field-mask parsing and trimming
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// metadata-field-name | the metadata field name to read the mask from | field-mask
///
/// @warning Masking messages that contain optional fields in protobuf prior to v3.13
/// causes a segmentation fault. If this is the case for you, you should not use
/// this middleware. See https://github.com/protocolbuffers/protobuf/issues/7801
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc server field-mask middleware component config

// clang-format on

class Component final : public MiddlewareComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// ugrpc::server::middlewares::field_mask::Component
    static constexpr std::string_view kName = "grpc-server-field-mask";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::shared_ptr<MiddlewareBase> GetMiddleware() override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::string metadata_field_name_;
};

}  // namespace ugrpc::server::middlewares::field_mask

USERVER_NAMESPACE_END
