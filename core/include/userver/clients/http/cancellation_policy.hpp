#pragma once

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

enum class CancellationPolicy {
    kIgnore,
    kCancel,
};

CancellationPolicy Parse(yaml_config::YamlConfig value, formats::parse::To<CancellationPolicy>);

}  // namespace clients::http

USERVER_NAMESPACE_END
