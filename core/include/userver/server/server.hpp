#pragma once

#include <userver/formats/json/value.hpp>

#include <userver/components/component_context.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/congestion_control/limiter.hpp>
#include <userver/server/congestion_control/sensor.hpp>
#include <userver/server/handlers/fallback_handlers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

namespace net {
struct StatsAggregation;
}

namespace http {
class HttpRequestHandler;
}

class RequestsView;

class ServerImpl;
struct ServerConfig;

class Server final : public congestion_control::Limitee,
                     public congestion_control::RequestsSource {
 public:
  Server(ServerConfig config, const storages::secdist::SecdistConfig& secdist,
         const components::ComponentContext& component_context);
  ~Server() override;

  const ServerConfig& GetConfig() const;

  std::vector<std::string> GetCommonMiddlewares() const;

  void WriteMonitorData(utils::statistics::Writer& writer) const;

  void WriteTotalHandlerStatistics(utils::statistics::Writer& writer) const;

  net::StatsAggregation GetServerStats() const;

  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

  size_t GetThrottlableHandlersCount() const;

  const http::HttpRequestHandler& GetHttpRequestHandler(
      bool is_monitor = false) const;

  void Start();

  void Stop();

  RequestsView& GetRequestsView();

  void SetLimit(std::optional<size_t> new_limit) override;

  void SetRpsRatelimit(std::optional<size_t> rps);

  void SetRpsRatelimitStatusCode(http::HttpStatus status_code);

  std::uint64_t GetTotalRequests() const override;

 private:
  std::unique_ptr<ServerImpl> pimpl;
};

}  // namespace server

USERVER_NAMESPACE_END
