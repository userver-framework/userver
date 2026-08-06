#include <storages/redis/impl/health_check_manager.hpp>

#include <userver/storages/redis/mock_client_base.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ConfigurableMockClient : public storages::redis::MockClientBase {
public:
    ConfigurableMockClient(bool is_ready = true)
        : is_ready_(is_ready)
    {}

    void SetIsReady(bool is_ready) { is_ready_ = is_ready; }
    bool IsReady(const storages::redis::HealthCheckParams&) const override { return is_ready_; }

private:
    bool is_ready_;
};

const storages::redis::HealthCheckParams kDefaultHealthCheckParams{storages::redis::WaitConnectedMode::kMaster, 0, 0};

}  // namespace

namespace storages::redis::impl {

TEST(HealthCheckManager, EmptyManagerReturnsOk) {
    HealthCheckManager manager;

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kOk);
}

TEST(HealthCheckManager, GetComponentHealthWithReadyClient) {
    HealthCheckManager manager;

    const std::string db_name = "test_db";
    manager.AddClient(db_name, kDefaultHealthCheckParams);

    auto mock_client = std::make_shared<ConfigurableMockClient>(true);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    clients[db_name] = mock_client;

    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kOk);
}

TEST(HealthCheckManager, GetComponentHealthWithNotReadyClient) {
    HealthCheckManager manager;

    const std::string db_name = "test_db";
    manager.AddClient(db_name, kDefaultHealthCheckParams);

    auto mock_client = std::make_shared<ConfigurableMockClient>(false);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    clients[db_name] = mock_client;

    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kFatal);
}

TEST(HealthCheckManager, GetComponentHealthWithMissingClient) {
    HealthCheckManager manager;

    const std::string db_name = "test_db";
    manager.AddClient(db_name, kDefaultHealthCheckParams);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kFatal);
}

TEST(HealthCheckManager, GetComponentHealthWithMissingSubscribeClient) {
    HealthCheckManager manager;

    const std::string db_name = "test_db";
    manager.AddSubscribeClient(db_name, kDefaultHealthCheckParams);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kFatal);
}

TEST(HealthCheckManager, GetComponentHealthWithMultipleClients) {
    HealthCheckManager manager;

    manager.AddClient("db1", kDefaultHealthCheckParams);
    manager.AddClient("db2", kDefaultHealthCheckParams);

    auto mock_client1 = std::make_shared<ConfigurableMockClient>(true);
    auto mock_client2 = std::make_shared<ConfigurableMockClient>(true);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    clients["db1"] = mock_client1;
    clients["db2"] = mock_client2;

    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kOk);
}

TEST(HealthCheckManager, GetComponentHealthWithOneFailedClient) {
    HealthCheckManager manager;

    manager.AddClient("db1", kDefaultHealthCheckParams);
    manager.AddClient("db2", kDefaultHealthCheckParams);

    auto mock_client1 = std::make_shared<ConfigurableMockClient>(true);
    auto mock_client2 = std::make_shared<ConfigurableMockClient>(false);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    clients["db1"] = mock_client1;
    clients["db2"] = mock_client2;

    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kFatal);
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
