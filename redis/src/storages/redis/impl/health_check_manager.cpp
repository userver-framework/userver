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
    auto calculate_health = [&] {
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

    const auto last_time_checked = last_time_checked_.load(std::memory_order_acquire);
    const auto now = std::chrono::steady_clock::now();
    if (now - last_time_checked < kCheckInterval) {
        return last_health_value_.load(std::memory_order_relaxed);
    }

    const auto health = calculate_health();
    last_health_value_.store(health, std::memory_order_relaxed);
    // Publish the timestamp after the health value. A reader that observes it
    // with acquire ordering also observes a valid cached health value. Parallel
    // refreshes may mix a timestamp and a health value from different calls;
    // this is acceptable because those calls happen close together.
    last_time_checked_.store(std::chrono::steady_clock::now(), std::memory_order_release);
    return health;
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
