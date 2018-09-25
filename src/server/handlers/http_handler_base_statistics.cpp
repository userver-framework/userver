#include <server/handlers/http_handler_base_statistics.hpp>

#include <boost/core/ignore_unused.hpp>

namespace server {
namespace handlers {

HttpHandlerBase::Statistics&
HttpHandlerBase::HandlerStatistics::GetStatisticByMethod(::http_method method) {
  /* HACK: C++14 doesn't think array::size() is constexpr in this context :-( */
  constexpr auto size = std::tuple_size<decltype(stats_by_method_)>::value;

  static_assert(HTTP_DELETE < size, "size is too low");
  static_assert(HTTP_GET < size, "size is too low");
  static_assert(HTTP_HEAD < size, "size is too low");
  static_assert(HTTP_POST < size, "size is too low");
  static_assert(HTTP_PUT < size, "size is too low");

  size_t index = static_cast<size_t>(method);
  boost::ignore_unused(index);
  assert(index < size);

  return stats_by_method_[method];
}

HttpHandlerBase::Statistics&
HttpHandlerBase::HandlerStatistics::GetTotalStatistics() {
  return stats_;
}

const std::vector<::http_method>&
HttpHandlerBase::HandlerStatistics::GetAllowedMethods() const {
  static std::vector<::http_method> methods{HTTP_DELETE, HTTP_GET, HTTP_HEAD,
                                            HTTP_POST, HTTP_PUT};
  return methods;
}

void HttpHandlerBase::HandlerStatistics::Account(http_method method,
                                                 unsigned int code,
                                                 std::chrono::milliseconds ms) {
  GetTotalStatistics().Account(code, ms.count());
  if (method <= stats_by_method_.size())
    GetStatisticByMethod(method).Account(code, ms.count());
}

}  // namespace handlers
}  // namespace server
