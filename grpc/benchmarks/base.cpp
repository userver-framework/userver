#include <utility>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/level.hpp>
#include <userver/ugrpc/tests/service.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/fixed_array.hpp>

#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

#include <benchmark/benchmark.h>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }

    ReadManyResult ReadMany(
        CallContext& /*context*/,
        sample::ugrpc::StreamGreetingRequest&& request,
        ReadManyWriter& writer
    ) override {
        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Hello again " + request.name());
        for (int i = 0; i < request.number(); ++i) {
            response.set_number(i);
            writer.Write(response);
        }
        return grpc::Status::OK;
    }

    WriteManyResult WriteMany(CallContext& /*context*/, WriteManyReader& reader) override {
        sample::ugrpc::StreamGreetingRequest request;
        int count = 0;
        while (reader.Read(request)) {
            ++count;
        }
        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Hello");
        response.set_number(count);
        return response;
    }

    ChatResult Chat(CallContext& /*context*/, ChatReaderWriter& stream) override {
        sample::ugrpc::StreamGreetingRequest request;
        sample::ugrpc::StreamGreetingResponse response;
        int count = 0;
        while (stream.Read(request)) {
            ++count;
            response.set_number(count);
            response.set_name("Hello " + request.name());
            stream.Write(response);
        }
        return grpc::Status::OK;
    }
};

template <typename GrpcService, bool Logging>
class TestService : public tests::ServiceBase {
public:
    template <typename... Args>
    TestService(Args&&... args) : tests::ServiceBase(std::forward<Args>(args)...) {
        if constexpr (Logging) {
            server::middlewares::log::Settings server_log_settings;
            server_log_settings.local_log_level = logging::Level::kInfo;
            server_log_settings.msg_log_level = logging::Level::kInfo;
            SetServerMiddlewares({std::make_shared<server::middlewares::log::Middleware>(server_log_settings)});

            client::middlewares::log::Settings client_log_settings;
            client_log_settings.log_level = logging::Level::kInfo;
            client_log_settings.msg_log_level = logging::Level::kInfo;
            SetClientMiddlewareFactories(
                {std::make_shared<client::middlewares::log::MiddlewareFactory>(client_log_settings)}
            );
        }
        RegisterService(service_);
        StartServer();
    }

    ~TestService() override { StopServer(); }

    GrpcService& GetService() { return service_; }

private:
    GrpcService service_{};
};

using GrpcClientTest = TestService<UnitTestService, false>;
using GrpcClientTestWithLogging = TestService<UnitTestService, true>;

std::unique_ptr<grpc::ClientContext> PrepareClientContext() {
    auto context = std::make_unique<grpc::ClientContext>();
    context->AddMetadata("req_header", "value");
    return context;
}

void UnaryRPCPayload(sample::ugrpc::UnitTestServiceClient& client) {
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto call = client.SayHello(out, PrepareClientContext());

    sample::ugrpc::GreetingResponse in;
    in = call.Finish();
    UINVARIANT("Hello " + out.name() == in.name(), "Behavior broken");
}

constexpr std::size_t kUnaryRPCPayloadRepeatedRepetitions = 256;

void UnaryRPCPayloadRepeated(sample::ugrpc::UnitTestServiceClient& client) {
    for (std::size_t i = 0; i < kUnaryRPCPayloadRepeatedRepetitions; ++i) {
        UnaryRPCPayload(client);
    }
}

void NewClientRepeated(GrpcClientTest& client_factory) {
    static constexpr std::size_t kRepetitions = 256;
    for (std::size_t i = 0; i < kRepetitions; ++i) {
        auto client = client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>();
    }
}

class NoopLogger : public logging::impl::LoggerBase {
public:
    NoopLogger() noexcept : LoggerBase(logging::Format::kRaw) { SetLevel(logging::Level::kInfo); }
    void Log(logging::Level, std::string_view) override {}
    void Flush() override {}
};

}  // namespace

void UnaryRPC(benchmark::State& state) {
    const logging::DefaultLoggerGuard logger_guard{std::make_shared<NoopLogger>()};

    engine::RunStandalone(state.range(0), [&] {
        GrpcClientTest client_factory;
        auto client = client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>();

        for (auto _ : state) {
            UnaryRPCPayload(client);
        }
    });
}

BENCHMARK(UnaryRPC)->DenseRange(1, 4)->Unit(benchmark::kMicrosecond);

void UnaryRPCWithLogging(benchmark::State& state) {
    const logging::DefaultLoggerGuard logger_guard{std::make_shared<NoopLogger>()};

    engine::RunStandalone(state.range(0), [&] {
        GrpcClientTestWithLogging client_factory;
        auto client = client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>();

        for (auto _ : state) {
            UnaryRPCPayload(client);
        }
    });
}

BENCHMARK(UnaryRPCWithLogging)->DenseRange(1, 4)->Unit(benchmark::kMicrosecond);

void UnaryRPCNewClient(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] {
        GrpcClientTest client_factory;

        for (auto _ : state) {
            auto client = client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>();
            UnaryRPCPayload(client);
        }
    });
}

BENCHMARK(UnaryRPCNewClient)->DenseRange(1, 4)->Unit(benchmark::kMicrosecond);

void BatchOfUnaryRPC(benchmark::State& state) {
    engine::RunStandalone(
        state.range(0),
        engine::TaskProcessorPoolsConfig{10000, 100000, 256 * 1024ULL, 1, "ev", false},
        [&] {
            static constexpr std::size_t kBatchSize = 16;
            GrpcClientTest client_factory;
            auto clients = utils::GenerateFixedArray(kBatchSize, [&client_factory](auto) {
                return client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>();
            });

            for (auto _ : state) {
                auto tasks = utils::GenerateFixedArray(kBatchSize, [&clients](auto i) {
                    return engine::AsyncNoSpan(UnaryRPCPayloadRepeated, std::ref(clients[i]));
                });
                engine::GetAll(tasks);
            }

            state.counters["rps"] = benchmark::Counter(
                static_cast<std::size_t>(state.iterations()) * kBatchSize * kUnaryRPCPayloadRepeatedRepetitions,
                benchmark::Counter::kIsRate
            );
        }
    );
}

BENCHMARK(BatchOfUnaryRPC)->DenseRange(1, 8)->Unit(benchmark::kMillisecond);

void BatchOfUnaryRPCNewClient(benchmark::State& state) {
    engine::RunStandalone(
        state.range(0),
        engine::TaskProcessorPoolsConfig{10000, 100000, 256 * 1024ULL, 1, "ev", false},
        [&] {
            static constexpr std::size_t kBatchSize = 16;
            GrpcClientTest client_factory;

            for (auto _ : state) {
                auto tasks = utils::GenerateFixedArray(kBatchSize, [&client_factory](auto) {
                    return engine::AsyncNoSpan([&client_factory] {
                        auto client = client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>();
                        UnaryRPCPayloadRepeated(client);
                    });
                });
                engine::GetAll(tasks);
            }
            state.counters["rps"] = benchmark::Counter(
                static_cast<std::size_t>(state.iterations()) * kBatchSize * kUnaryRPCPayloadRepeatedRepetitions,
                benchmark::Counter::kIsRate
            );
        }
    );
}

BENCHMARK(BatchOfUnaryRPCNewClient)->DenseRange(1, 8)->Unit(benchmark::kMillisecond);

void BatchOfNewClient(benchmark::State& state) {
    engine::RunStandalone(
        state.range(0),
        engine::TaskProcessorPoolsConfig{10000, 100000, 256 * 1024ULL, 1, "ev", false},
        [&] {
            static constexpr std::size_t kBatchSize = 16;
            GrpcClientTest client_factory;

            for (auto _ : state) {
                auto tasks = utils::GenerateFixedArray(kBatchSize, [&client_factory](auto) {
                    return engine::AsyncNoSpan(NewClientRepeated, std::ref(client_factory));
                });
                engine::GetAll(tasks);
            }
        }
    );
}

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
BENCHMARK(BatchOfNewClient)->DenseRange(1, 1)->Unit(benchmark::kMillisecond);
#endif
BENCHMARK(BatchOfNewClient)->DenseRange(1, 8)->Unit(benchmark::kMillisecond);
#else
BENCHMARK(BatchOfNewClient)->DenseRange(1, 8)->Unit(benchmark::kMillisecond);
#endif

}  // namespace ugrpc

USERVER_NAMESPACE_END
