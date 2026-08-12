#include <storages/redis/impl/health_check_manager.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <storages/redis/subscribe_client_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

void HealthCheckManager::AddClient(const std::string& db_name, HealthCheckParams params) { clients_[db_name] = params; }

void HealthCheckManager::AddSubscribeClient(const std::string& db_name, HealthCheckParams params) {
    subscribe_clients_[db_name] = params;
}

void HealthCheckManager::RemoveClient(const std::string& db_name) { clients_.erase(db_name); }

void HealthCheckManager::RemoveSubscribeClient(const std::string& db_name) { subscribe_clients_.erase(db_name); }

components::ComponentHealth HealthCheckManager::GetComponentHealth(
    const std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>>& clients,
    const std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>>& subscribe_clients
) const {
    static const auto kCheckInterval = std::chrono::seconds(1);
    auto calcluate_health = [&] {
        for (const auto& [db_name, params] : clients_) {
            const auto client_it = clients.find(db_name);
            if (client_it == clients.end()) {
                LOG_WARNING() << "required redis client " << db_name << " not found";
                return components::ComponentHealth::kFatal;
            }
            if (!client_it->second->IsReady(params)) {
                LOG_WARNING() << "required redis client " << db_name << " is not ready";
                return components::ComponentHealth::kFatal;
            }
        }

        for (const auto& [db_name, params] : subscribe_clients_) {
            const auto client_it = subscribe_clients.find(db_name);
            if (client_it == subscribe_clients.end()) {
                LOG_WARNING() << "required redis subscribe client " << db_name << " not found";
                return components::ComponentHealth::kFatal;
            }
            if (!client_it->second->IsReady(params)) {
                LOG_WARNING() << "required redis subscribe client " << db_name << " is not ready";
                return components::ComponentHealth::kFatal;
            }
        }
        return components::ComponentHealth::kOk;
    };

    const auto now = std::chrono::system_clock::now();
    if (now - last_time_checked_ < kCheckInterval) {
        return last_health_value_;
    }

    last_health_value_ = calcluate_health();
    return last_health_value_;
}

void HealthCheckManager::WriteHealthStatistics(
    utils::statistics::Writer& writer,
    const std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>>& clients
) const {
    for (const auto& [name, params] : clients_) {
        const auto client_it = clients.find(name);
        if (client_it != clients.end()) {
            const auto is_healthy = client_it->second->IsReady(params);
            writer.ValueWithLabels(is_healthy ? 1 : 0, {"redis_database", name});
        }
    }
}

void HealthCheckManager::WriteSubscribeHealthStatistics(
    utils::statistics::Writer& writer,
    const std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>>& clients
) const {
    for (const auto& [name, params] : subscribe_clients_) {
        const auto client_it = clients.find(name);
        if (client_it != clients.end()) {
            const auto is_healthy = client_it->second->IsReady(params);
            writer.ValueWithLabels(is_healthy ? 1 : 0, {"redis_database", name});
        }
    }
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
