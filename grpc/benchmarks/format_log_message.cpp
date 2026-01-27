#include <ugrpc/server/impl/format_log_message.hpp>

#include <userver/logging/log_extra.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>

#include <benchmark/benchmark.h>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

void FormatLogMessage(benchmark::State& state) {
    const std::multimap<grpc::string_ref, grpc::string_ref> metadata = {
        {"user-agent", "grpc-go/1.45.0"},
    };
    std::string peer = "2a02:aaaa:aaaa:aaaa::1:1f";
    std::chrono::system_clock::time_point start_time(std::chrono::seconds{1024 * 1024 * 42});
    std::string_view call_name = "hello.HelloService/SayHello";

    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    for (auto _ : state) {
        auto result =
            ugrpc::server::impl::FormatLogMessage(metadata, peer, start_time, call_name, grpc::StatusCode::OK, nullptr);
        benchmark::DoNotOptimize(result);
    }

    auto result =
        ugrpc::server::impl::FormatLogMessage(metadata, peer, start_time, call_name, grpc::StatusCode::OK, nullptr);

    const std::string_view log_line_view{result.ExtractTextLogItem().log_line};  // Convert SmallString to string_view

    UINVARIANT(utils::text::StartsWith(log_line_view, "tskv\ttimestamp=1971-05-"), "Fail");
    UINVARIANT(log_line_view.find("\ttimezone=") != std::string::npos, "Fail 1");
    UINVARIANT(
        log_line_view
                .find("\tuser_agent=grpc-go/1.45.0\t"
                      "ip=2a02:aaaa:aaaa:aaaa::1:1f\t"
                      "x_real_ip=2a02:aaaa:aaaa:aaaa::1:1f\t"
                      "request=hello.HelloService/SayHello\t"
                      "request_time=") != std::string::npos,
        "Fail 2"
    );

    UINVARIANT(log_line_view.find("\tgrpc_status=0\t") != std::string::npos, "Fail 3");
    UINVARIANT(log_line_view.find("\tgrpc_status_code=OK\n") != std::string::npos, "Fail 4");
}

BENCHMARK(FormatLogMessage);

USERVER_NAMESPACE_END
