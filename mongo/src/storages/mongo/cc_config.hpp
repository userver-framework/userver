#pragma once

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

const dynamic_config::Key<congestion_control::v2::Config> kCcConfig{
    "MONGO_CONGESTION_CONTROL_SETTINGS",
    dynamic_config::DefaultAsJsonString{"{}"}};

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
