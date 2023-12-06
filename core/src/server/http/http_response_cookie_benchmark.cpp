#include <benchmark/benchmark.h>

#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/small_string.hpp>

USERVER_NAMESPACE_BEGIN

void http_cookie_serialization(benchmark::State& state) {
  auto cookie = server::http::Cookie::FromString(
      "name1=value1; Domain=domain.com; Path=/; Expires=Wed, 12 Jun 2019 "
      "16:51:45 GMT; Max-Age=3600; Secure; SameSite=None; HttpOnly");
  USERVER_NAMESPACE::http::headers::HeadersString os;
  for ([[maybe_unused]] auto _ : state) {
    cookie.value().AppendToString(os);
    os.clear();
  }
}

BENCHMARK(http_cookie_serialization);

USERVER_NAMESPACE_END
