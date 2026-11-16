#include <userver/ugrpc/impl/event_engine.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/slice.h>

#include <userver/engine/async.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <engine/tests/task_processor_utils.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

class EchoService final : public grpc::CallbackGenericService {
public:
    explicit EchoService(engine::TaskProcessor* expected)
        : expected_(expected)
    {}

    grpc::ServerGenericBidiReactor* CreateReactor(grpc::GenericCallbackServerContext*) override {
        if (expected_) {
            EXPECT_TRUE(engine::current_task::IsTaskProcessorThread());
            if (engine::current_task::IsTaskProcessorThread()) {
                EXPECT_EQ(&engine::current_task::GetTaskProcessor(), expected_);
                ++callbacks_on_target;
            }
        }
        class Reactor final : public grpc::ServerGenericBidiReactor {
        public:
            Reactor() { StartRead(&message_); }
            void OnReadDone(bool ok) override {
                if (ok) {
                    StartWrite(&message_);
                } else {
                    Finish(grpc::Status::OK);
                }
            }
            void OnWriteDone(bool ok) override {
                if (ok) {
                    message_.Clear();
                    StartRead(&message_);
                } else {
                    Finish(grpc::Status{grpc::StatusCode::CANCELLED, "write failed"});
                }
            }
            void OnDone() override { delete this; }

        private:
            grpc::ByteBuffer message_;
        };
        return new Reactor;
    }

    std::atomic<unsigned> callbacks_on_target{0};

private:
    engine::TaskProcessor* expected_;
};

bool Next(grpc::CompletionQueue& queue, void* expected) {
    void* tag{};
    bool ok{};
    const auto result = queue.AsyncNext(&tag, &ok, std::chrono::system_clock::now() + utest::kMaxTestWaitTime);
    UINVARIANT(result == grpc::CompletionQueue::GOT_EVENT, "RPC completion timed out");
    EXPECT_EQ(tag, expected);
    return ok;
}

void CheckEcho(int port, const std::vector<std::string>& messages) {
    const auto channel =
        grpc::CreateChannel(("127.0.0.1:" + std::to_string(port)).c_str(), grpc::InsecureChannelCredentials());
    grpc::GenericStub stub{channel};
    grpc::CompletionQueue queue;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + utest::kMaxTestWaitTime);
    auto call = stub.PrepareCall(&context, "/factory.Echo/Exchange", &queue);
    const utils::FastScopeGuard cleanup([&]() noexcept {
        context.TryCancel();
        call.reset();
        queue.Shutdown();
        void* tag{};
        bool ok{};
        while (queue.Next(&tag, &ok)) {
        }
    });
    ASSERT_TRUE(call);
    int tag{};
    call->StartCall(&tag);
    ASSERT_TRUE(Next(queue, &tag));
    for (const auto& message : messages) {
        const grpc::Slice slice{message.data(), message.size()};
        grpc::ByteBuffer request{&slice, 1};
        call->Write(request, &tag);
        ASSERT_TRUE(Next(queue, &tag));
        grpc::ByteBuffer response;
        call->Read(&response, &tag);
        ASSERT_TRUE(Next(queue, &tag));
        std::vector<grpc::Slice> slices;
        ASSERT_TRUE(response.Dump(&slices).ok());
        std::string received;
        for (const auto& part : slices) {
            received.append(reinterpret_cast<const char*>(part.begin()), part.size());
        }
        EXPECT_EQ(received, message);
    }
    call->WritesDone(&tag);
    ASSERT_TRUE(Next(queue, &tag));
    grpc::ByteBuffer eof;
    call->Read(&eof, &tag);
    EXPECT_FALSE(Next(queue, &tag));
    grpc::Status status;
    call->Finish(&status, &tag);
    ASSERT_TRUE(Next(queue, &tag));
    EXPECT_TRUE(status.ok()) << status.error_message();
}

TEST(GrpcEventEngineFactory, CallbackRpcAndShutdownWorkWithOneWorkerAndTheSameBlockingProcessor) {
    EnsureRuntimeConfigured();
    EXPECT_TRUE(IsRuntimeConfigured());
    engine::tests::TwoStandaloneTaskProcessors processors;
    EchoService service{&processors.GetMain()};
    std::unique_ptr<PosixEventEngineOwner> owner;
    processors.RunBlocking([&] {
        EXPECT_EQ(&engine::current_task::GetBlockingTaskProcessor(), &processors.GetMain());
        owner = engine::CriticalAsyncNoTracing(engine::current_task::GetBlockingTaskProcessor(), [&] {
                    return CreateEventEngine(processors.GetMain());
                }).Get();
    });
    const auto ee = GetPosixEventEngine(*owner);
    const utils::FastScopeGuard stop_owner([&]() noexcept {
        processors.RunBlocking([&] {
            engine::CriticalAsyncNoTracing(engine::current_task::GetBlockingTaskProcessor(), [&] {
                owner.reset();
            }).Get();
        });
        EXPECT_TRUE(ee->IsStopped());
    });
    int port = 0;
    grpc::ServerBuilder builder;
    SetEventEngine(builder, owner->GetEventEngine());
    builder.RegisterCallbackGenericService(&service);
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    auto server = builder.BuildAndStart();
    const utils::FastScopeGuard stop_server([&]() noexcept {
        if (server) {
            server->Shutdown();
        }
        server.reset();
    });
    ASSERT_TRUE(server);
    ASSERT_GT(port, 0);
    CheckEcho(port, {"hello"});
    CheckEcho(port, {std::string(64 * 1024, 'a'), std::string(1024 * 1024, 'b'), "tail"});
    EXPECT_EQ(service.callbacks_on_target, 2);
}

TEST(GrpcEventEngineFactory, SeparateOwnersKeepTheirTaskProcessorsAndLifetimes) {
    engine::tests::TwoStandaloneTaskProcessors processors;
    std::unique_ptr<PosixEventEngineOwner> first;
    std::unique_ptr<PosixEventEngineOwner> second;
    processors.RunBlocking([&] {
        first = CreateEventEngine(processors.GetMain());
        second = CreateEventEngine(processors.GetSecondary());
    });
    auto first_engine = GetPosixEventEngine(*first);
    auto second_engine = GetPosixEventEngine(*second);
    std::atomic<int> calls{0};
    first_engine->Run([&] {
        EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &processors.GetMain());
        ++calls;
    });
    second_engine->Run([&] {
        EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &processors.GetSecondary());
        ++calls;
    });
    processors.RunBlocking([&] { first.reset(); });
    EXPECT_TRUE(first_engine->IsStopped());
    EXPECT_FALSE(second_engine->IsStopping());
    second_engine->Run([&] { ++calls; });
    processors.RunBlocking([&] { second.reset(); });
    EXPECT_EQ(calls, 3);
    EXPECT_TRUE(second_engine->IsStopped());
}

}  // namespace
}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
