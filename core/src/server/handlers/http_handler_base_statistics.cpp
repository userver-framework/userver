#include <server/handlers/http_handler_base_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

HttpHandlerMethodStatistics& HttpHandlerStatistics::GetStatisticByMethod(
    http::HttpMethod method) {
  auto index = static_cast<size_t>(method);
  UASSERT(index < stats_by_method_.size());

  return stats_by_method_[index];
}

const HttpHandlerMethodStatistics& HttpHandlerStatistics::GetStatisticByMethod(
    http::HttpMethod method) const {
  auto index = static_cast<size_t>(method);
  UASSERT(index < stats_by_method_.size());

  return stats_by_method_[index];
}

HttpHandlerMethodStatistics& HttpHandlerStatistics::GetTotalStatistics() {
  return stats_;
}

const HttpHandlerMethodStatistics& HttpHandlerStatistics::GetTotalStatistics()
    const {
  return stats_;
}

bool HttpHandlerStatistics::IsOkMethod(http::HttpMethod method) const {
  return static_cast<size_t>(method) < stats_by_method_.size();
}

void HttpHandlerStatistics::Account(http::HttpMethod method, unsigned int code,
                                    std::chrono::milliseconds ms) {
  GetTotalStatistics().Account(code, ms.count());
  if (IsOkMethod(method))
    GetStatisticByMethod(method).Account(code, ms.count());
}

HttpHandlerStatisticsScope::HttpHandlerStatisticsScope(
    HttpHandlerStatistics& stats, http::HttpMethod method,
    server::http::HttpResponse& response)
    : stats_(stats),
      method_(method),
      start_time_(std::chrono::system_clock::now()),
      response_(response) {
  stats_.GetTotalStatistics().IncrementInFlight();
  if (stats_.IsOkMethod(method))
    stats_.GetStatisticByMethod(method).IncrementInFlight();
}

HttpHandlerStatisticsScope::~HttpHandlerStatisticsScope() {
  const auto finish_time = std::chrono::system_clock::now();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      finish_time - start_time_);
  Account(static_cast<int>(response_.GetStatus()), ms);
}

void HttpHandlerStatisticsScope::Account(unsigned int code,
                                         std::chrono::milliseconds ms) {
  stats_.Account(method_, code, ms);

  stats_.GetTotalStatistics().DecrementInFlight();
  if (stats_.IsOkMethod(method_))
    stats_.GetStatisticByMethod(method_).DecrementInFlight();
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
