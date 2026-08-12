#pragma once

#include <atomic>

#include <server/http/http_request_handler.hpp>
#include <server/net/http1_connection.hpp>
#include <server/net/listener_config.hpp>
#include <userver/engine/single_consumer_event.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

struct EndpointInfo {
    EndpointInfo(const ListenerConfig&, http::HttpRequestHandler&);

    const ListenerConfig& listener_config;
    http::HttpRequestHandler& request_handler;

    std::atomic<size_t> connection_count{0};
    engine::SingleConsumerEvent no_connections_event;
};

}  // namespace server::net

USERVER_NAMESPACE_END
