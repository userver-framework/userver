#pragma once

/// @file userver/clients/http/standalone_client.hpp
/// @brief @copybrief clients::http::CreateStandaloneHttpClient()

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/config.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

struct StandaloneConfig final {
    bool multiplexing_enabled{false};
    std::optional<size_t> max_host_connections{std::nullopt};
};

/// @brief Creates HTTP client with given settings and middlewares
std::shared_ptr<Client> CreateStandaloneHttpClient(
    ClientSettings settings,
    StandaloneConfig standalone_config,
    engine::TaskProcessor& fs_task_processor,
    std::vector<utils::NotNull<clients::http::MiddlewareBase*>> middlewares = {}
);

}  // namespace clients::http

USERVER_NAMESPACE_END
