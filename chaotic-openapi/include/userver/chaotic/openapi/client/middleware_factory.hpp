#pragma once

#include <memory>
#include <userver/chaotic/openapi/client/middleware.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

class MiddlewareFactory : public components::ComponentBase {
public:
    using components::ComponentBase::ComponentBase;

    virtual ~MiddlewareFactory() = default;

    virtual std::shared_ptr<Middleware> Create(const USERVER_NAMESPACE::yaml_config::YamlConfig& config) = 0;

    virtual std::string GetStaticConfigSchemaStr() = 0;
};

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
