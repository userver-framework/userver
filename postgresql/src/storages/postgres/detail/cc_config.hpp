#pragma once

#include <userver/congestion_control/controllers/linear.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

inline const dynamic_config::Key<congestion_control::v2::Config> kCcConfig{
    "POSTGRES_CONGESTION_CONTROL_SETTINGS",
    dynamic_config::DefaultAsJsonString{"{}"},
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
