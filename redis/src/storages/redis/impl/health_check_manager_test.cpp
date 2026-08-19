#include <storages/redis/impl/health_check_manager.hpp>

#include <atomic>
#include <chrono>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/storages/redis/mock_client_base.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ConfigurableMockClient : public storages::redis::MockClientBase {
public:
    explicit ConfigurableMockClient(
        bool is_ready = true,
        std::chrono::milliseconds is_ready_delay = std::chrono::milliseconds{0}
    )
        : is_ready_(is_ready),
          is_ready_delay_(is_ready_delay)
    {}

    void SetIsReady(bool is_ready) { is_ready_.store(is_ready); }

    std::size_t GetIsReadyCallCount() const { return is_ready_call_count_.load(); }

    bool IsReady(const storages::redis::HealthCheckParams&) const override {
        is_ready_call_count_.fetch_add(1);
        engine::SleepFor(is_ready_delay_);
        return is_ready_.load();
    }

private:
    std::atomic<bool> is_ready_;
    const std::chrono::milliseconds is_ready_delay_;
    mutable std::atomic<std::size_t> is_ready_call_count_{0};
};

const storages::redis::HealthCheckParams kDefaultHealthCheckParams{storages::redis::WaitConnectedMode::kMaster, 0, 0};
constexpr std::size_t kThreadCount = 8;

}  // namespace

namespace storages::redis::impl {

UTEST(HealthCheckManager, EmptyManagerReturnsOk) {
    HealthCheckManager manager;

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kOk);
}

UTEST(HealthCheckManager, GetComponentHealthWithReadyClient) {
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

UTEST(HealthCheckManager, CachesComponentHealth) {
    HealthCheckManager manager;

    const std::string db_name = "test_db";
    manager.AddClient(db_name, kDefaultHealthCheckParams);

    auto mock_client = std::make_shared<ConfigurableMockClient>(true);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    clients[db_name] = mock_client;

    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    EXPECT_EQ(manager.GetComponentHealth(clients, subscribe_clients), components::ComponentHealth::kOk);

    mock_client->SetIsReady(false);

    EXPECT_EQ(manager.GetComponentHealth(clients, subscribe_clients), components::ComponentHealth::kOk);
    EXPECT_EQ(mock_client->GetIsReadyCallCount(), 1);
}

UTEST(HealthCheckManager, ConcurrentGetComponentHealth) {
    HealthCheckManager manager;

    const std::string db_name = "test_db";
    manager.AddClient(db_name, kDefaultHealthCheckParams);

    auto mock_client = std::make_shared<ConfigurableMockClient>(true, std::chrono::milliseconds{50});

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    clients[db_name] = mock_client;

    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto tasks = utils::GenerateFixedArray(kThreadCount, [&](std::size_t) {
        return engine::AsyncNoTracing([&] { return manager.GetComponentHealth(clients, subscribe_clients); });
    });

    for (auto& task : tasks) {
        EXPECT_EQ(task.Get(), components::ComponentHealth::kOk);
    }
    EXPECT_LE(mock_client->GetIsReadyCallCount(), kThreadCount);
}

UTEST(HealthCheckManager, GetComponentHealthWithNotReadyClient) {
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

UTEST(HealthCheckManager, GetComponentHealthWithMissingClient) {
    HealthCheckManager manager;

    const std::string db_name = "test_db";
    manager.AddClient(db_name, kDefaultHealthCheckParams);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kFatal);
}

UTEST(HealthCheckManager, GetComponentHealthWithMissingSubscribeClient) {
    HealthCheckManager manager;

    const std::string db_name = "test_db";
    manager.AddSubscribeClient(db_name, kDefaultHealthCheckParams);

    std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>> clients;
    std::unordered_map<std::string, std::shared_ptr<storages::redis::SubscribeClientImpl>> subscribe_clients;

    auto health = manager.GetComponentHealth(clients, subscribe_clients);
    EXPECT_EQ(health, components::ComponentHealth::kFatal);
}

UTEST(HealthCheckManager, GetComponentHealthWithMultipleClients) {
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

UTEST(HealthCheckManager, GetComponentHealthWithOneFailedClient) {
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
