#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/health_check_param.hpp>

#include <userver/components/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Writer;

}  // namespace utils::statistics

namespace storages::redis {

class SubscribeClientImpl;

}  // namespace storages::redis

namespace storages::redis::impl {

class HealthCheckManager {
public:
    void AddClient(const std::string& db_name, HealthCheckParams params);
    void AddSubscribeClient(const std::string& db_name, HealthCheckParams params);
    void RemoveClient(const std::string& db_name);
    void RemoveSubscribeClient(const std::string& db_name);

    components::ComponentHealth GetComponentHealth(
        const std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>>& clients,
        const std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>>& subscribe_clients
    ) const;

    void WriteHealthStatistics(
        utils::statistics::Writer& writer,
        const std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>>& clients
    ) const;

    void WriteSubscribeHealthStatistics(
        utils::statistics::Writer& writer,
        const std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>>& clients
    ) const;

private:
    std::unordered_map<std::string, HealthCheckParams> clients_;
    std::unordered_map<std::string, HealthCheckParams> subscribe_clients_;
    mutable components::ComponentHealth last_health_value_{components::ComponentHealth::kFatal};
    mutable std::chrono::system_clock::time_point last_time_checked_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
