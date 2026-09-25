#pragma once

/// @file userver/server/request/impl/client_metrics_shard.hpp
/// @brief Client metrics shard set by server::request::SetClientMetricsShard

#include <string>
#include <vector>

#include <userver/engine/task/inherited_variable.hpp>
#include <userver/utils/statistics/labels.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request::impl {

struct ClientMetricsShard final {
    std::string path;
    std::vector<utils::statistics::Label> labels;
};

extern engine::TaskInheritedVariable<ClientMetricsShard> kClientMetricsShard;

}  // namespace server::request::impl

USERVER_NAMESPACE_END
