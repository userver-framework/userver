#include <userver/server/server.hpp>

#include <atomic>
#include <shared_mutex>
#include <stdexcept>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>
#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/http/http_request_handler.hpp>
#include <server/net/endpoint_info.hpp>
#include <server/net/listener.hpp>
#include <server/net/stats.hpp>
#include <server/requests_view.hpp>
#include <server/server_config.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/middlewares/configuration.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

namespace {

struct PortInfo final {
    void Init(
        const ServerConfig& config,
        const net::ListenerConfig& listener_config,
        const components::ComponentContext& component_context,
        bool is_monitor
    );

    void Start();

    void Stop();

    bool IsRunning() const noexcept;

    std::optional<http::HttpRequestHandler> request_handler;
    std::shared_ptr<net::EndpointInfo> endpoint_info;
    request::ResponseDataAccounter data_accounter;
    std::vector<net::Listener> listeners;
};

void PortInfo::Init(
    const ServerConfig& config,
    const net::ListenerConfig& listener_config,
    const components::ComponentContext& component_context,
    bool is_monitor
) {
    LOG_DEBUG() << "Creating listener" << (is_monitor ? " (monitor)" : "");

    engine::TaskProcessor& task_processor = listener_config.task_processor
                                                ? component_context.GetTaskProcessor(*listener_config.task_processor)
                                                : engine::current_task::GetTaskProcessor();

    request_handler.emplace(
        component_context, config.logger_access, config.logger_access_tskv, is_monitor, config.server_name
    );

    endpoint_info = std::make_shared<net::EndpointInfo>(listener_config, *request_handler);

    const auto& event_thread_pool = task_processor.EventThreadPool();
    size_t listener_shards = listener_config.shards ? *listener_config.shards : event_thread_pool.GetSize();

    listeners.reserve(listener_shards);
    while (listener_shards--) {
        listeners.emplace_back(endpoint_info, task_processor, data_accounter);
    }
}

void PortInfo::Start() {
    UASSERT(request_handler);
    request_handler->DisableAddHandler();
    for (auto& listener : listeners) {
        listener.Start();
    }
}

void PortInfo::Stop() {
    LOG_TRACE() << "Stopping listeners";
    listeners.clear();
    LOG_TRACE() << "Stopped listeners";

    if (endpoint_info) {
        UASSERT_MSG(endpoint_info->connection_count == 0, "Not all the connections were closed");
    }

    LOG_TRACE() << "Stopping request handlers";
    request_handler.reset();
    LOG_TRACE() << "Stopped request handlers";
}

bool PortInfo::IsRunning() const noexcept { return request_handler && request_handler->IsAddHandlerDisabled(); }

void WriteRateAndLegacyMetrics(utils::statistics::Writer&& writer, utils::statistics::Rate metric) {
    writer = metric.value;
    writer["v2"] = metric;
}

}  // namespace

class ServerImpl final {
public:
    ServerImpl(
        ServerConfig config,
        const storages::secdist::SecdistConfig& secdist,
        const components::ComponentContext& component_context
    );
    ~ServerImpl();

    void StartPortInfos();
    void Stop();

    void AddHandler(const handlers::HttpHandlerBase& handler, engine::TaskProcessor& task_processor);

    std::size_t GetThrottlableHandlersCount() const;
    std::chrono::milliseconds GetAvgRequestTimeMs() const;
    const http::HttpRequestHandler& GetHttpRequestHandler(bool is_monitor) const;
    net::StatsAggregation GetServerStats() const;
    const ServerConfig& GetServerConfig() const { return config_; }
    const std::vector<std::string>& GetMiddlewares() const;

    RequestsView& GetRequestsView();
    void WriteTotalHandlerStatistics(utils::statistics::Writer& writer) const;

    void SetRpsRatelimitStatusCode(http::HttpStatus status_code);
    void SetRpsRatelimit(std::optional<size_t> rps);
    std::uint64_t GetTotalRequests() const;

private:
    PortInfo main_port_info_;
    PortInfo monitor_port_info_;

    std::atomic<size_t> throttlable_handlers_count_{0};

    mutable std::shared_mutex on_stop_mutex_{};
    bool is_stopping_{false};

    std::atomic<bool> has_requests_view_watchers_{false};
    RequestsView requests_view_{};

    ServerConfig config_;
    std::vector<std::string> middlewares_;
};

ServerImpl::ServerImpl(
    ServerConfig config,
    const storages::secdist::SecdistConfig& secdist,
    const components::ComponentContext& component_context
)
    : config_(std::move(config)) {
    LOG_DEBUG() << "Creating server";

    for (auto& port : config_.listener.ports) port.ReadTlsSettings(secdist);

    main_port_info_.Init(config_, config_.listener, component_context, false);
    if (config_.max_response_size_in_flight) {
        main_port_info_.data_accounter.SetMaxLevel(*config_.max_response_size_in_flight);
    }
    if (config_.monitor_listener) {
        monitor_port_info_.Init(config_, *config_.monitor_listener, component_context, true);
    }

    middlewares_ = component_context.FindComponent<middlewares::PipelineBuilder>(config_.middleware_pipeline_builder)
                       .BuildPipeline(middlewares::DefaultPipeline());

    LOG_INFO() << "Server is created, listening for incoming connections.";
}

ServerImpl::~ServerImpl() { Stop(); }

void ServerImpl::StartPortInfos() {
    UASSERT(main_port_info_.request_handler);

    if (has_requests_view_watchers_.load()) {
        auto queue = requests_view_.GetQueue();
        requests_view_.StartBackgroundWorker();
        auto hook = [queue](std::shared_ptr<http::HttpRequest> request) mutable { queue->enqueue(std::move(request)); };
        main_port_info_.request_handler->SetNewRequestHook(hook);
        if (monitor_port_info_.request_handler) {
            monitor_port_info_.request_handler->SetNewRequestHook(hook);
        }
    }

    main_port_info_.Start();
    if (monitor_port_info_.request_handler) {
        monitor_port_info_.Start();
    } else {
        LOG_WARNING() << "No 'listener-monitor' in 'server' component";
    }
}

void ServerImpl::Stop() {
    {
        const std::lock_guard lock{on_stop_mutex_};
        if (is_stopping_) return;
        is_stopping_ = true;
    }

    LOG_INFO() << "Stopping server";
    main_port_info_.Stop();
    monitor_port_info_.Stop();
    LOG_INFO() << "Stopped server";
}

void ServerImpl::AddHandler(const handlers::HttpHandlerBase& handler, engine::TaskProcessor& task_processor) {
    UASSERT(!main_port_info_.IsRunning());

    if (handler.IsMonitor() && !monitor_port_info_.request_handler) {
        throw std::logic_error(
            "Attempt to register a handler for 'listener-monitor' that was not "
            "configured in 'server' section of the component config"
        );
    }

    if (handler.IsMonitor()) {
        UINVARIANT(
            monitor_port_info_.request_handler,
            "Attempt to register monitor handler while the server has no "
            "'listener-monitor'"
        );
        monitor_port_info_.request_handler->AddHandler(handler, task_processor);
    } else {
        UASSERT(main_port_info_.request_handler);
        main_port_info_.request_handler->AddHandler(handler, task_processor);
    }

    if (!handler.IsMonitor()) {
        if (handler.GetConfig().throttling_enabled) {
            throttlable_handlers_count_++;
        }
    }
}

std::size_t ServerImpl::GetThrottlableHandlersCount() const {
    UASSERT(main_port_info_.IsRunning());
    return throttlable_handlers_count_.load();
}

std::chrono::milliseconds ServerImpl::GetAvgRequestTimeMs() const {
    return main_port_info_.data_accounter.GetAvgRequestTime();
}

const http::HttpRequestHandler& ServerImpl::GetHttpRequestHandler(bool is_monitor) const {
    if (is_monitor) {
        UASSERT(monitor_port_info_.request_handler);
        return *monitor_port_info_.request_handler;
    }

    UASSERT(main_port_info_.request_handler);
    return *main_port_info_.request_handler;
}

net::StatsAggregation ServerImpl::GetServerStats() const {
    net::StatsAggregation summary;

    const std::shared_lock lock{on_stop_mutex_};
    if (is_stopping_) return summary;
    for (const auto& listener : main_port_info_.listeners) {
        summary += listener.GetStats();
    }

    return summary;
}

const std::vector<std::string>& ServerImpl::GetMiddlewares() const { return middlewares_; }

RequestsView& ServerImpl::GetRequestsView() {
    UASSERT(!main_port_info_.IsRunning() || has_requests_view_watchers_.load());

    has_requests_view_watchers_.store(true);
    return requests_view_;
}

void Server::SetLimit(std::optional<size_t> new_limit) { SetRpsRatelimit(new_limit); }

void ServerImpl::WriteTotalHandlerStatistics(utils::statistics::Writer& writer) const {
    handlers::HttpHandlerStatisticsSnapshot total;

    {
        // Protect against main_port_info_.request_handler_.reset() in Stop()
        const std::shared_lock lock{on_stop_mutex_};
        if (is_stopping_) {
            return;
        }

        UASSERT(main_port_info_.request_handler);
        const auto& handlers = main_port_info_.request_handler->GetHandlerInfoIndex().GetHandlers();

        for (const auto handler_ptr : handlers) {
            for (const auto method : handler_ptr->GetAllowedMethods()) {
                total.Add(handlers::HttpHandlerStatisticsSnapshot{
                    handler_ptr->GetHandlerStatistics().GetByMethod(method)});
            }
        }
    }

    writer = total;
}

void ServerImpl::SetRpsRatelimitStatusCode(http::HttpStatus status_code) {
    UASSERT(main_port_info_.request_handler);
    main_port_info_.request_handler->SetRpsRatelimitStatusCode(status_code);
}

void ServerImpl::SetRpsRatelimit(std::optional<size_t> rps) {
    UASSERT(main_port_info_.request_handler);
    main_port_info_.request_handler->SetRpsRatelimit(rps);
}

std::uint64_t ServerImpl::GetTotalRequests() const {
    const auto stats = GetServerStats();
    return stats.active_request_count + stats.requests_processed_count.value;
}

Server::Server(
    ServerConfig config,
    const storages::secdist::SecdistConfig& secdist,
    const components::ComponentContext& component_context
)
    : pimpl_(std::make_unique<ServerImpl>(std::move(config), secdist, component_context)) {}

Server::~Server() = default;

const ServerConfig& Server::GetConfig() const { return pimpl_->GetServerConfig(); }

std::vector<std::string> Server::GetCommonMiddlewares() const { return pimpl_->GetMiddlewares(); }

void Server::WriteMonitorData(utils::statistics::Writer& writer) const {
    const auto server_stats = pimpl_->GetServerStats();
    if (auto conn_stats = writer["connections"]) {
        conn_stats["active"] = server_stats.active_connections;
        WriteRateAndLegacyMetrics(conn_stats["opened"], server_stats.connections_created);
        WriteRateAndLegacyMetrics(conn_stats["closed"], server_stats.connections_closed);
    }

    if (auto request_stats = writer["requests"]) {
        request_stats["active"] = server_stats.active_request_count;
        request_stats["avg-lifetime-ms"] = pimpl_->GetAvgRequestTimeMs().count();
        WriteRateAndLegacyMetrics(request_stats["processed"], server_stats.requests_processed_count);
        request_stats["parsing"] = server_stats.parser_stats.parsing_request_count;

        if (auto http2_request_stats = request_stats["http2"]) {
            http2_request_stats["streams-count"] = server_stats.parser_stats.streams_count;
            http2_request_stats["streams-parse-error"] = server_stats.parser_stats.streams_parse_error;
            http2_request_stats["streams-close"] = server_stats.parser_stats.streams_close;
            http2_request_stats["reset-streams"] = server_stats.parser_stats.reset_streams;
            http2_request_stats["goaway"] = server_stats.parser_stats.goaway;
        }
    }
}

void Server::WriteTotalHandlerStatistics(utils::statistics::Writer& writer) const {
    pimpl_->WriteTotalHandlerStatistics(writer);
}

net::StatsAggregation Server::GetServerStats() const { return pimpl_->GetServerStats(); }

void Server::AddHandler(const handlers::HttpHandlerBase& handler, engine::TaskProcessor& task_processor) {
    pimpl_->AddHandler(handler, task_processor);
}

size_t Server::GetThrottlableHandlersCount() const { return pimpl_->GetThrottlableHandlersCount(); }

const http::HttpRequestHandler& Server::GetHttpRequestHandler(bool is_monitor) const {
    return pimpl_->GetHttpRequestHandler(is_monitor);
}

void Server::Start() {
    LOG_INFO() << "Starting server";
    pimpl_->StartPortInfos();
    LOG_INFO() << "Server is started";
}

void Server::Stop() { pimpl_->Stop(); }

RequestsView& Server::GetRequestsView() { return pimpl_->GetRequestsView(); }

void Server::SetRpsRatelimit(std::optional<size_t> rps) { pimpl_->SetRpsRatelimit(rps); }

void Server::SetRpsRatelimitStatusCode(http::HttpStatus status_code) { pimpl_->SetRpsRatelimitStatusCode(status_code); }

std::uint64_t Server::GetTotalRequests() const { return pimpl_->GetTotalRequests(); }

}  // namespace server

USERVER_NAMESPACE_END
