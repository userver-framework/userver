#include <server/net/connection_config.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

Http2SessionConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<Http2SessionConfig>) {
    Http2SessionConfig conf{};
    conf.max_concurrent_streams = value["max_concurrent_streams"].As<std::uint32_t>(conf.max_concurrent_streams);
    conf.max_frame_size = value["max_frame_size"].As<std::uint32_t>(conf.max_frame_size);
    conf.initial_window_size = value["initial_window_size"].As<std::uint32_t>(conf.initial_window_size);
    return conf;
}

ConnectionConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<ConnectionConfig>) {
    ConnectionConfig config;

    config.in_buffer_size = value["in_buffer_size"].As<size_t>(config.in_buffer_size);
    config.requests_queue_size_threshold =
        value["requests_queue_size_threshold"].As<size_t>(config.requests_queue_size_threshold);
    config.keepalive_timeout = value["keepalive_timeout"].As<std::chrono::seconds>(config.keepalive_timeout);

    if (!value["stream_close_check_delay"].IsMissing()) {
        config.abort_check_delay = utils::StringToDuration(value["stream_close_check_delay"].As<std::string>());
    }

    config.http_version = value["http-version"].As<USERVER_NAMESPACE::http::HttpVersion>(config.http_version);

    config.http2_session_config = value["http2-session"].As<Http2SessionConfig>(config.http2_session_config);

    return config;
}

}  // namespace server::net

USERVER_NAMESPACE_END
