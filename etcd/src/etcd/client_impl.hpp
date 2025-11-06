#pragma once

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/etcd/client.hpp>
#include <userver/etcd/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

namespace impl {

class ClientImpl : public Client {
public:
    ClientImpl(clients::http::Client& http_client, ClientSettings settings);

    void Put(std::string_view key, std::string_view value) override;

    [[nodiscard]] std::optional<std::string> Get(std::string_view key) override;

    [[nodiscard]] std::vector<KeyValueState> Range(std::string_view key_prefix) override;

    void Delete(std::string_view key) override;

    WatchListener StartWatch(std::string_view key) override;

private:
    clients::http::Response
    PerformEtcdRequest(const std::function<std::string(std::string_view)>& url_builder, std::string_view data);

    [[nodiscard]] clients::http::StreamedResponse
    PerformStreamedEtcdRequest(const std::function<std::string(std::string_view)>& url_builder, std::string_view data);

    void WatchKeyChanges(const std::string key, concurrent::SpscQueue<KeyValueState>::Producer producer);

    using WatchQueuePtr = std::shared_ptr<concurrent::SpscQueue<KeyValueState>>;
    clients::http::Client& http_client_;
    concurrent::Variable<std::vector<WatchQueuePtr>> watch_queues_;
    concurrent::BackgroundTaskStorage bts_;
    const ClientSettings settings_;
};

}  // namespace impl

}  // namespace etcd

USERVER_NAMESPACE_END
