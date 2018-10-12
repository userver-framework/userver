#include <server/handlers/http_handler_base_statistics.hpp>

namespace server {
namespace handlers {

HttpHandlerBase::Statistics&
HttpHandlerBase::HandlerStatistics::GetStatisticByMethod(
    http::HttpMethod method) {
  size_t index = static_cast<size_t>(method);
  assert(index < stats_by_method_.size());

  return stats_by_method_[index];
}

HttpHandlerBase::Statistics&
HttpHandlerBase::HandlerStatistics::GetTotalStatistics() {
  return stats_;
}

bool HttpHandlerBase::HandlerStatistics::IsOkMethod(
    http::HttpMethod method) const {
  return static_cast<size_t>(method) < stats_by_method_.size();
}

void HttpHandlerBase::HandlerStatistics::Account(http::HttpMethod method,
                                                 unsigned int code,
                                                 std::chrono::milliseconds ms) {
  GetTotalStatistics().Account(code, ms.count());
  if (IsOkMethod(method))
    GetStatisticByMethod(method).Account(code, ms.count());
}

HttpHandlerBase::HttpHandlerStatisticsScope::HttpHandlerStatisticsScope(
    HttpHandlerBase::HandlerStatistics& stats, http::HttpMethod method)
    : stats_(stats), method_(method) {
  stats_.GetTotalStatistics().IncrementInFlight();
  if (stats_.IsOkMethod(method))
    stats_.GetStatisticByMethod(method).IncrementInFlight();
}

void HttpHandlerBase::HttpHandlerStatisticsScope::Account(
    unsigned int code, std::chrono::milliseconds ms) {
  stats_.Account(method_, code, ms);

  stats_.GetTotalStatistics().DecrementInFlight();
  if (stats_.IsOkMethod(method_))
    stats_.GetStatisticByMethod(method_).DecrementInFlight();
}

}  // namespace handlers
}  // namespace server
