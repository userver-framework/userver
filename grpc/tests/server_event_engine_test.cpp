#include <userver/ugrpc/server/server.hpp>

#include <utility>

#include <userver/engine/task/current_task.hpp>
#include <userver/ugrpc/tests/service.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/invariant_error.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include <grpcpp/impl/grpc_library.h>
#include <gtest/gtest.h>

#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/function_ref.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#endif

USERVER_NAMESPACE_BEGIN

namespace {

ugrpc::server::ServerConfig MakeEventEngineConfig() {
    return ugrpc::server::ServerConfig{.use_callback_api = true, .event_engine = {.enabled = true}};
}

void CreateServer(ugrpc::server::ServerConfig&& config) { const ugrpc::tests::ServiceBase server{std::move(config)}; }

}  // namespace

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, RequiresCallbackApi) {
    constexpr char kErrorMessage[] = "requires 'use-callback-api: true'";
    auto config = MakeEventEngineConfig();
    config.use_callback_api = false;
#ifdef NDEBUG
    UEXPECT_THROW_MSG(CreateServer(std::move(config)), utils::InvariantError, kErrorMessage);
#else
    EXPECT_DEATH(CreateServer(std::move(config)), kErrorMessage);
#endif
}

UTEST(GrpcServerEventEngineOption, TaskProcessorAloneDoesNotEnableEventEngine) {
    ugrpc::server::ServerConfig config{
        .use_callback_api = true,
        .event_engine = {.task_processor = &engine::current_task::GetTaskProcessor()},
    };
    UEXPECT_NO_THROW(CreateServer(std::move(config)));
}

#if defined(ARCADIA_ROOT) && defined(__linux__)

namespace {

constexpr unsigned kChildTimeoutSeconds = 60;
constexpr int kOpenStreamStopCount = 10;
constexpr int kMessageCount = 2;
constexpr int kFinalResponseNumber = 42;
constexpr char kMessageName[] = "event-engine-test";
constexpr char kNextingThreadName[] = "nexting_thread";
constexpr char kStartFailureMessage[] = "See grpcpp logs for details";

using Client = sample::ugrpc::UnitTestServiceClient;

sample::ugrpc::StreamGreetingRequest MakeStreamRequest(int number) {
    sample::ugrpc::StreamGreetingRequest request;
    request.set_number(number);
    return request;
}

sample::ugrpc::StreamGreetingResponse MakeStreamResponse(int number) {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_number(number);
    return response;
}

class EchoService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext&, sample::ugrpc::GreetingRequest&& request) override {
        sample::ugrpc::GreetingResponse response;
        response.set_name(request.name());
        return response;
    }

    ChatResult Chat(CallContext&, ChatReaderWriter& stream) override {
        sample::ugrpc::StreamGreetingRequest request;
        while (stream.Read(request)) {
            stream.Write(MakeStreamResponse(request.number()));
        }
        return MakeStreamResponse(kFinalResponseNumber);
    }
};

using EchoServer = ugrpc::tests::Service<EchoService>;

// Without SO_REUSEPORT a second server cannot share the listening port of the first one.
ugrpc::server::ServerConfig MakeExclusivePortConfig(int port) {
    auto config = MakeEventEngineConfig();
    config.port = port;
    config.channel_args = {{"grpc.so_reuseport", "0"}};
    return config;
}

void CheckUnaryCall(const Client& client) {
    sample::ugrpc::GreetingRequest request;
    request.set_name(kMessageName);
    EXPECT_EQ(client.SayHello(request).name(), kMessageName);
}

void CheckBidiExchange(const Client& client) {
    auto stream = client.Chat();
    sample::ugrpc::StreamGreetingResponse response;
    for (int number = 0; number < kMessageCount; ++number) {
        EXPECT_TRUE(stream.Write(MakeStreamRequest(number)));
        ASSERT_TRUE(stream.Read(response));
        EXPECT_EQ(response.number(), number);
    }
    EXPECT_TRUE(stream.WritesDone());
    ASSERT_TRUE(stream.Read(response));
    EXPECT_EQ(response.number(), kFinalResponseNumber);
    EXPECT_FALSE(stream.Read(response));
}

bool HasThread(std::string_view name) {
    for (const auto& task : std::filesystem::directory_iterator{"/proc/self/task"}) {
        std::ifstream comm{task.path() / "comm"};
        std::string thread_name;
        if (std::getline(comm, thread_name) && thread_name == name) {
            return true;
        }
    }
    return false;
}

[[noreturn]] void RunAndExit(utils::function_ref<void()> scenario) {
    ::alarm(kChildTimeoutSeconds);
    scenario();
    // gtest does not print the failures of a death test's child process.
    const auto& result = *testing::UnitTest::GetInstance()->current_test_info()->result();
    for (int i = 0; i < result.total_part_count(); ++i) {
        if (result.GetTestPartResult(i).failed()) {
            std::cerr << result.GetTestPartResult(i);
        }
    }
    std::_Exit(testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
}

// gRPC settings are process-wide and can be applied only once, so each scenario needs a process of its own.
void RunInChildProcess(utils::function_ref<void()> scenario) {
    EXPECT_EXIT(RunAndExit(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}

void ExpectStartFailure(ugrpc::tests::ServiceBase& server) {
#ifdef NDEBUG
    UEXPECT_THROW_MSG(server.StartServer(), utils::InvariantError, kStartFailureMessage);
#else
    server.StartServer();
#endif
}

}  // namespace

UTEST(GrpcServerEventEngineOption, CallbackApiUsesNextingThreadsWhenUnset) {
    EchoServer server({.server_config = {.use_callback_api = true}});
    CheckUnaryCall(server.MakeClient<Client>());
    EXPECT_TRUE(HasThread(kNextingThreadName));
}

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, ServesRpcsWithoutNextingThreads) {
    RunInChildProcess([] {
        EchoServer server({.server_config = MakeEventEngineConfig()});

        const auto client = server.MakeClient<Client>();
        CheckUnaryCall(client);
        CheckBidiExchange(client);
        EXPECT_FALSE(HasThread(kNextingThreadName));
    });
}

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, EmptyServerRespondsUnimplemented) {
    RunInChildProcess([] {
        ugrpc::tests::ServiceBase server{MakeEventEngineConfig()};
        server.StartServer();
        UEXPECT_THROW(server.MakeClient<Client>().SayHello({}), ugrpc::client::UnimplementedError);
        server.StopServer();
    });
}

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, RejectsLateConfiguration) {
    EXPECT_EXIT(
        RunAndExit([] {
            const grpc::internal::GrpcLibrary grpc_library;
            CreateServer(MakeEventEngineConfig());
        }),
        testing::KilledBySignal(SIGABRT),
        ""
    );
    EXPECT_EXIT(
        RunAndExit([] {
            {
                const grpc::internal::GrpcLibrary grpc_library;
            }
            CreateServer(MakeEventEngineConfig());
        }),
        testing::KilledBySignal(SIGABRT),
        ""
    );
}

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, RejectsUnsupportedPollStrategy) {
    constexpr char kErrorMessage[] = "requires gRPC's epoll1 I/O poller";
    const auto scenario = [] {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        ASSERT_EQ(::setenv("GRPC_POLL_STRATEGY", "poll", 1), 0);
        CreateServer(MakeEventEngineConfig());
    };
#ifdef NDEBUG
    RunInChildProcess([&] { UEXPECT_THROW_MSG(scenario(), utils::InvariantError, kErrorMessage); });
#else
    EXPECT_DEATH(RunAndExit(scenario), kErrorMessage);
#endif
}

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, StopsInEveryLifecycleState) {
    RunInChildProcess([] {
        EchoService service;
        CreateServer(MakeEventEngineConfig());
        {
            ugrpc::tests::ServiceBase stopped_before_start{MakeEventEngineConfig()};
            stopped_before_start.StopServer();
            stopped_before_start.StopServer();
        }
        {
            ugrpc::tests::ServiceBase stopped_after_serving{MakeEventEngineConfig()};
            stopped_after_serving.RegisterService(service);
            stopped_after_serving.StartServer();
            stopped_after_serving.GetServer().StopServing();
            stopped_after_serving.StopServer();
        }
        {
            ugrpc::tests::ServiceBase destroyed_after_serving{MakeEventEngineConfig()};
            destroyed_after_serving.RegisterService(service);
            destroyed_after_serving.StartServer();
            destroyed_after_serving.GetServer().StopServing();
        }
        {
            ugrpc::tests::ServiceBase destroyed_while_active{MakeEventEngineConfig()};
            destroyed_while_active.RegisterService(service);
            destroyed_while_active.StartServer();
        }
        {
            ugrpc::tests::ServiceBase stopped_in_cancelled_task{MakeEventEngineConfig()};
            stopped_in_cancelled_task.RegisterService(service);
            stopped_in_cancelled_task.StartServer();
            engine::AsyncNoTracing([&stopped_in_cancelled_task] {
                engine::current_task::RequestCancel();
                stopped_in_cancelled_task.StopServer();
            }).Get();
        }
#ifdef NDEBUG
        {
            constexpr int kInvalidPort = 70000;
            auto config = MakeEventEngineConfig();
            config.port = kInvalidPort;
            UEXPECT_THROW(CreateServer(std::move(config)), utils::InvariantError);
        }
#endif
        EchoServer server({.server_config = MakeEventEngineConfig()});
        CheckUnaryCall(server.MakeClient<Client>());
    });
}

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, StartFailureThrowsAndReleasesListeners) {
    const auto scenario = [] {
        EchoService service;
        ugrpc::tests::ServiceBase listening{MakeExclusivePortConfig(0)};
        listening.RegisterService(service);
        listening.StartServer();
        const int port = listening.GetServer().GetPort();
        {
            ugrpc::tests::ServiceBase busy_port{MakeExclusivePortConfig(port)};
            busy_port.RegisterService(service);
            ExpectStartFailure(busy_port);
            busy_port.StopServer();
            busy_port.StopServer();
        }
        {
            const auto directory = fs::blocking::TempDirectory::Create("/tmp", "userver-ee-test-");
            auto config = MakeExclusivePortConfig(port);
            config.unix_socket_path = directory.GetPath() + "/server.sock";
            ugrpc::tests::ServiceBase unix_socket_and_busy_port{std::move(config)};
            unix_socket_and_busy_port.RegisterService(service);
            ExpectStartFailure(unix_socket_and_busy_port);
        }
        listening.StopServer();

        ugrpc::tests::ServiceBase same_port{MakeExclusivePortConfig(port)};
        same_port.RegisterService(service);
        same_port.StartServer();
        CheckUnaryCall(same_port.MakeClient<Client>());
        same_port.StopServer();
    };
#ifdef NDEBUG
    RunInChildProcess(scenario);
#else
    EXPECT_DEATH(RunAndExit(scenario), kStartFailureMessage);
#endif
}

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, StopsWithOpenStream) {
    RunInChildProcess([] {
        for (int number = 0; number < kOpenStreamStopCount; ++number) {
            EchoServer server({.server_config = MakeEventEngineConfig()});
            const auto client = server.MakeClient<Client>();
            auto stream = client.Chat();
            sample::ugrpc::StreamGreetingResponse response;
            ASSERT_TRUE(stream.Write(MakeStreamRequest(number)));
            ASSERT_TRUE(stream.Read(response));
            server.GetServer().StopServing();
        }
    });
}

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, CoexistsWithOtherServers) {
    RunInChildProcess([] {
        EchoServer first({.server_config = MakeEventEngineConfig()});
        auto second_config = MakeEventEngineConfig();
        second_config.event_engine.task_processor = &engine::current_task::GetTaskProcessor();
        EchoServer second({.server_config = std::move(second_config)});
        EchoServer callback_api({.server_config = {.use_callback_api = true}});
        EchoServer completion_queue_api;
        CheckUnaryCall(first.MakeClient<Client>());
        CheckUnaryCall(second.MakeClient<Client>());
        CheckUnaryCall(callback_api.MakeClient<Client>());
        CheckUnaryCall(completion_queue_api.MakeClient<Client>());

        first.StopServer();
        CheckUnaryCall(second.MakeClient<Client>());
        CheckUnaryCall(callback_api.MakeClient<Client>());
        CheckUnaryCall(completion_queue_api.MakeClient<Client>());
    });
}

#else

UTEST_DEATH(GrpcServerEventEngineOptionDeathTest, IsRejectedInBuildsWithoutEventEngine) {
    EXPECT_UINVARIANT_FAILURE_MSG(CreateServer(MakeEventEngineConfig()), "not supported in this build");
}

#endif

USERVER_NAMESPACE_END
