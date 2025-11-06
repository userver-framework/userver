#pragma once

/// @file userver/etcd/client.hpp
/// @brief @copybrief etcd::Client

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <userver/clients/http/component.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/etcd/key_value_state.hpp>
#include <userver/etcd/settings.hpp>
#include <userver/etcd/watch_listener.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <schemas/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

/// @brief Etcd client implemented using http client
class Client {
public:
    virtual ~Client() = default;

    /// @brief Puts a key value pair into etcd cluster.
    /// The pair should be retrieve only with current client,
    /// because Put can transform the key.
    ///
    virtual void Put(std::string_view key, std::string_view value) = 0;

    /// @brief Gets a value from etcd cluster by a key.
    /// If there is no value with such key, returns std::nullopt
    ///
    [[nodiscard]] virtual std::optional<std::string> Get(std::string_view key) = 0;

    /// @brief Retrieves key values pairs from the etcd cluster,
    /// the keys of which match the passed prefix.
    /// If there is no value with such key, returns empty vector
    ///
    [[nodiscard]] virtual std::vector<KeyValueState> Range(std::string_view key_prefix) = 0;

    /// @brief Delete a key value pair with the passed key
    /// from the etcd cluster
    ///
    virtual void Delete(std::string_view key) = 0;

    /// @brief Start task that produces events when key value pair changes
    ///
    virtual WatchListener StartWatch(std::string_view key) = 0;
};

using ClientPtr = std::shared_ptr<Client>;

}  // namespace etcd

USERVER_NAMESPACE_END
