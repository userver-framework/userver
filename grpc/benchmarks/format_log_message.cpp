#include <userver/ugrpc/server/rpc.hpp>

#include <userver/utils/text.hpp>

#include <benchmark/benchmark.h>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

void FormatLogMessage(benchmark::State& state) {
  const std::multimap<grpc::string_ref, grpc::string_ref> metadata = {
      {"user-agent", "grpc-go/1.45.0"},
  };
  std::string peer = "2a02:aaaa:aaaa:aaaa::1:1f";
  std::chrono::system_clock::time_point start_time(
      std::chrono::seconds{1024 * 1024 * 42});
  std::string_view call_name = "hello.HelloService/SayHello";

  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    auto result = ugrpc::server::impl::FormatLogMessage(
        metadata, peer, start_time, call_name, grpc::StatusCode::OK);
    benchmark::DoNotOptimize(result);
  }

  auto result = ugrpc::server::impl::FormatLogMessage(
      metadata, peer, start_time, call_name, grpc::StatusCode::OK);

  UINVARIANT(utils::text::StartsWith(result, "tskv\ttimestamp=1971-05-"),
             "Fail");
  UINVARIANT(result.find("\ttimezone=") != std::string::npos, "Fail 1");
  UINVARIANT(result.find("\tuser_agent=grpc-go/1.45.0\t"
                         "ip=2a02:aaaa:aaaa:aaaa::1:1f\t"
                         "x_real_ip=2a02:aaaa:aaaa:aaaa::1:1f\t"
                         "request=hello.HelloService/SayHello\t"
                         "upstream_response_time_ms=") != std::string::npos,
             "Fail 2");

  UINVARIANT(result.find("\tgrpc_status=0\t") != std::string::npos, "Fail 3");
  UINVARIANT(result.find("\tgrpc_status_code=OK\n") != std::string::npos,
             "Fail 4");
}

BENCHMARK(FormatLogMessage);

USERVER_NAMESPACE_END
