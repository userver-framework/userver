#include <server/handlers/http_handler_base_statistics.hpp>

#include <boost/core/ignore_unused.hpp>

namespace server {
namespace handlers {

HttpHandlerBase::Statistics&
HttpHandlerBase::HandlerStatistics::GetStatisticByMethod(
    http::HttpMethod method) {
  /* HACK: C++14 doesn't think array::size() is constexpr in this context :-( */
  constexpr auto size = std::tuple_size<decltype(stats_by_method_)>::value;

  static_assert(static_cast<size_t>(http::HttpMethod::kDelete) < size,
                "size is too low");
  static_assert(static_cast<size_t>(http::HttpMethod::kGet) < size,
                "size is too low");
  static_assert(static_cast<size_t>(http::HttpMethod::kHead) < size,
                "size is too low");
  static_assert(static_cast<size_t>(http::HttpMethod::kPost) < size,
                "size is too low");
  static_assert(static_cast<size_t>(http::HttpMethod::kPut) < size,
                "size is too low");
  static_assert(static_cast<size_t>(http::HttpMethod::kPatch) < size,
                "size is too low");

  size_t index = static_cast<size_t>(method);
  assert(index < size);

  return stats_by_method_[index];
}

HttpHandlerBase::Statistics&
HttpHandlerBase::HandlerStatistics::GetTotalStatistics() {
  return stats_;
}

const std::vector<http::HttpMethod>&
HttpHandlerBase::HandlerStatistics::GetAllowedMethods() const {
  static std::vector<http::HttpMethod> methods{
      http::HttpMethod::kDelete, http::HttpMethod::kGet,
      http::HttpMethod::kHead,   http::HttpMethod::kPost,
      http::HttpMethod::kPut,    http::HttpMethod::kPatch};
  return methods;
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
