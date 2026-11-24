#include <google/protobuf/struct.pb.h>
#include <grpcpp/support/status.h>

#include <userver/compiler/impl/tsan.hpp>
#include <userver/compiler/thread_local.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/protobuf/arena.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/server/impl/call_processor.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/result.hpp>
#include <userver/ugrpc/tests/service.hpp>
#include <userver/utest/utest.hpp>

#include <tests/proto2_service.pb.h>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Response = sample::ugrpc::GreetingResponse;

struct ArenaState {
    std::optional<std::reference_wrapper<const google::protobuf::Message>> request;
    std::optional<std::reference_wrapper<const google::protobuf::Message>> response;
    std::optional<std::reference_wrapper<const google::protobuf::Message>> sent_response;
    engine::SingleConsumerEvent call_finished;
    engine::SingleConsumerEvent arena_destroyed;
    std::atomic<int> destroyed{0};
    std::atomic<int> finished{0};
    bool retain_arena{false};
    bool heap_responses{false};
    std::shared_ptr<google::protobuf::Arena> shared_arena;
};

struct ArenaCleanup {
    ArenaState& state;
    ~ArenaCleanup() {
        ++state.destroyed;
        state.arena_destroyed.Send();
    }
};

class ArenaMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    explicit ArenaMiddleware(ArenaState& state)
        : state_(state)
    {}

    void PostRecvMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& message)
        const override {
        const auto arena = context.GetArena();
        EXPECT_EQ(message.GetArena(), arena ? &*arena : nullptr);
        state_.request.emplace(message);
    }

    void PreSendMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& message)
        const override {
        const auto arena = context.GetArena();
        EXPECT_EQ(message.GetArena(), arena && !state_.heap_responses ? &*arena : nullptr);
        if (state_.response) {
            EXPECT_EQ(&message, &state_.response->get());
        }
        const auto* field = message.GetDescriptor()->FindFieldByName("name");
        ASSERT_NE(field, nullptr);
        message.GetReflection()->SetString(&message, field, "middleware");
        state_.sent_response.emplace(message);
    }

    void OnCallFinish(ugrpc::server::MiddlewareCallContext& context, const std::optional<grpc::Status>& status)
        const override {
        EXPECT_EQ(state_.destroyed, 0);
        if (ugrpc::IsSingleRequestMethod(context.GetRpcType())) {
            ASSERT_TRUE(state_.request);
            EXPECT_NE(state_.request->get().GetDescriptor(), nullptr);
        }
        if (status && status->ok()) {
            ASSERT_TRUE(state_.sent_response);
            const auto& message = state_.sent_response->get();
            const auto* field = message.GetDescriptor()->FindFieldByName("name");
            ASSERT_NE(field, nullptr);
            EXPECT_EQ(message.GetReflection()->GetString(message, field), "middleware");
        }
        if (state_.retain_arena) {
            const auto arena = context.GetArena();
            state_.shared_arena = context.GetArenaSharedPtr();
            EXPECT_EQ(static_cast<bool>(state_.shared_arena), arena.has_value());
            if (arena) {
                EXPECT_EQ(state_.shared_arena.get(), &*arena);
            }
            EXPECT_EQ(context.GetArena().has_value(), arena.has_value());
            EXPECT_EQ(context.GetArenaSharedPtr(), state_.shared_arena);
            auto shared_arena = context.GetArenaSharedPtr();
            shared_arena.reset();
            EXPECT_EQ(state_.destroyed, 0);
        }
        ++state_.finished;
        state_.call_finished.Send();
    }

private:
    ArenaState& state_;
};

class ArenaService final : public sample::ugrpc::UnitTestServiceBase {
public:
    explicit ArenaService(ArenaState& state)
        : state_(state)
    {}

    SayHelloResult SayHello(CallContext& context, sample::ugrpc::GreetingRequest&& request) override {
        Prepare(context);
        EXPECT_EQ(&request, &state_.request->get());
        CheckArena(request, context.GetArena());
        if (request.name() == "error") {
            return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "error"};
        }
        if (request.name() == "throw") {
            throw std::runtime_error("handler failure");
        }
        if (request.name() == "cancel") {
            const engine::TaskCancellationBlocker blocker;
            auto& server_context = context.GetServerContext();
            server_context.TryCancel();
            const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
            while (!server_context.IsCancelled() && !deadline.IsReached()) {
                engine::Yield();
            }
            EXPECT_TRUE(server_context.IsCancelled());
        }
        if (state_.heap_responses) {
            return Response{};
        }
        return MakeResponse<SayHelloResult, Response>(context);
    }

    ReadManyResult ReadMany(
        CallContext& context,
        sample::ugrpc::StreamGreetingRequest&& request,
        ReadManyWriter& writer
    ) override {
        Prepare(context);
        EXPECT_EQ(&request, &state_.request->get());
        CheckArena(request, context.GetArena());
        for (int i = 0; i < request.number(); ++i) {
            WriteResponse(context, writer, i);
        }
        return MakeResponse<ReadManyResult, sample::ugrpc::StreamGreetingResponse>(context);
    }

    WriteManyResult WriteMany(CallContext& context, WriteManyReader& reader) override {
        Prepare(context);
        ReadRequests(context, reader);
        return MakeResponse<WriteManyResult, sample::ugrpc::StreamGreetingResponse>(context);
    }

    ChatResult Chat(CallContext& context, ChatReaderWriter& stream) override {
        Prepare(context);
        ReadRequests(context, stream);
        WriteResponse(context, stream, 1);
        return MakeResponse<ChatResult, sample::ugrpc::StreamGreetingResponse>(context);
    }

private:
    void Prepare(CallContext& context) {
        if (auto arena = context.GetArena()) {
            arena->Own(new ArenaCleanup{state_});
        }
    }

    static void CheckArena(
        const google::protobuf::Message& message,
        utils::OptionalRef<google::protobuf::Arena> arena
    ) {
        EXPECT_EQ(message.GetArena(), arena ? &*arena : nullptr);
    }

    template <typename Message>
    Message& MessageOrStorage(CallContext& context, Message& storage) {
        if (auto arena = context.GetArena()) {
            return *protobuf::CreateArenaMessage<Message>(*arena);
        }
        return storage;
    }

    template <typename Result, typename Message>
    Result MakeResponse(CallContext& context) {
        Message storage;
        auto& message = MessageOrStorage(context, storage);
        if (context.GetArena()) {
            state_.response.emplace(message);
            return Result::BorrowArenaResponse(message);
        }
        return std::move(message);
    }

    template <typename Writer>
    void WriteResponse(CallContext& context, Writer& writer, int number) {
        sample::ugrpc::StreamGreetingResponse storage;
        auto& message = MessageOrStorage(context, storage);
        if (context.GetArena()) {
            state_.response.emplace(message);
        }
        message.set_number(number);
        writer.Write(std::move(message));
        state_.sent_response.reset();
    }

    template <typename Reader>
    void ReadRequests(CallContext& context, Reader& reader) {
        sample::ugrpc::StreamGreetingRequest storage;
        auto& message = MessageOrStorage(context, storage);
        while (reader.Read(message)) {
            EXPECT_EQ(&state_.request->get(), &message);
            CheckArena(message, context.GetArena());
        }
        state_.request.reset();
    }

    ArenaState& state_;
};

struct ArenaOptions {
    std::initializer_list<bool> arenas{false, true};
    bool heap_responses{false};
};

template <typename Func>
void WithArenaService(Func&& func, ArenaOptions options = {}) {
    for (const auto callback : {false, true}) {
        for (const auto arena : options.arenas) {
            SCOPED_TRACE(::testing::Message() << "callback=" << callback << " arena=" << arena);
            ArenaState state;
            state.heap_responses = options.heap_responses;
            ugrpc::tests::Service<ArenaService> service(
                {.server_config = {.use_callback_api = callback},
                 .server_middlewares = {std::make_shared<ArenaMiddleware>(state)},
                 .use_protobuf_arena = arena},
                std::in_place,
                state
            );
            auto client = service.MakeClient<sample::ugrpc::UnitTestServiceClient>();
            func(client);
            EXPECT_TRUE(state.call_finished.WaitForEventFor(utest::kMaxTestWaitTime));
            if (arena) {
                EXPECT_TRUE(state.arena_destroyed.WaitForEventFor(utest::kMaxTestWaitTime));
            }
            EXPECT_EQ(state.finished, 1);
            EXPECT_EQ(state.destroyed, arena ? 1 : 0);
        }
    }
}

}  // namespace

UTEST(GrpcArena, Unary) {
    WithArenaService([](auto& client) { EXPECT_EQ(client.SayHello({}).name(), "middleware"); });
}

UTEST(GrpcArena, ServerStreaming) {
    WithArenaService([](auto& client) {
        sample::ugrpc::StreamGreetingRequest request;
        request.set_number(3);
        auto stream = client.ReadMany(request);
        sample::ugrpc::StreamGreetingResponse response;
        int count = 0;
        while (stream.Read(response)) {
            EXPECT_EQ(response.name(), "middleware");
            ++count;
        }
        EXPECT_EQ(count, 4);
    });
}

UTEST(GrpcArena, ClientStreaming) {
    WithArenaService([](auto& client) {
        auto stream = client.WriteMany();
        EXPECT_TRUE(stream.Write({}));
        EXPECT_TRUE(stream.Write({}));
        EXPECT_EQ(stream.Finish().name(), "middleware");
    });
}

UTEST(GrpcArena, BidirectionalStreaming) {
    WithArenaService([](auto& client) {
        auto stream = client.Chat();
        EXPECT_TRUE(stream.Write({}));
        EXPECT_TRUE(stream.WritesDone());
        sample::ugrpc::StreamGreetingResponse response;
        int count = 0;
        while (stream.Read(response)) {
            EXPECT_EQ(response.name(), "middleware");
            ++count;
        }
        EXPECT_EQ(count, 2);
    });
}

UTEST(GrpcArena, Error) {
    WithArenaService(
        [](auto& client) {
            sample::ugrpc::GreetingRequest request;
            request.set_name("error");
            UEXPECT_THROW(client.SayHello(request), ugrpc::client::InvalidArgumentError);
        },
        {.arenas = {true}}
    );
}

UTEST(GrpcArena, HandlerException) {
    WithArenaService(
        [](auto& client) {
            sample::ugrpc::GreetingRequest request;
            request.set_name("throw");
            UEXPECT_THROW(client.SayHello(request), ugrpc::client::UnknownError);
        },
        {.arenas = {true}}
    );
}

UTEST(GrpcArena, Cancellation) {
    WithArenaService(
        [](auto& client) {
            sample::ugrpc::GreetingRequest request;
            request.set_name("cancel");
            UEXPECT_THROW(client.SayHello(request), ugrpc::client::CancelledError);
        },
        {.arenas = {true}}
    );
}

UTEST(GrpcArena, HeapResponseWithArena) {
    WithArenaService(
        [](auto& client) { EXPECT_EQ(client.SayHello({}).name(), "middleware"); },
        {.arenas = {true}, .heap_responses = true}
    );
}

UTEST(GrpcArena, ShareIndependentArenas) {
    for (const auto callback : {false, true}) {
        for (const auto arena : {false, true}) {
            SCOPED_TRACE(::testing::Message() << "callback=" << callback << " arena=" << arena);
            ArenaState state;
            state.retain_arena = true;
            std::shared_ptr<google::protobuf::Arena> first_arena;
            decltype(state.request) first_request;
            decltype(state.sent_response) first_response;
            sample::ugrpc::GreetingRequest request;
            request.set_name("retained request");
            {
                ugrpc::tests::Service<ArenaService> service(
                    {.server_config = {.use_callback_api = callback},
                     .server_middlewares = {std::make_shared<ArenaMiddleware>(state)},
                     .use_protobuf_arena = arena},
                    std::in_place,
                    state
                );
                auto client = service.MakeClient<sample::ugrpc::UnitTestServiceClient>();
                EXPECT_EQ(client.SayHello(request).name(), "middleware");
                ASSERT_TRUE(state.call_finished.WaitForEventFor(utest::kMaxTestWaitTime));
                first_arena = std::move(state.shared_arena);
                first_request = state.request;
                first_response = state.sent_response;
                EXPECT_EQ(client.SayHello(request).name(), "middleware");
                ASSERT_TRUE(state.call_finished.WaitForEventFor(utest::kMaxTestWaitTime));
            }
            EXPECT_EQ(state.finished, 2);
            EXPECT_EQ(state.destroyed, 0);
            if (arena) {
                ASSERT_TRUE(first_arena);
                ASSERT_TRUE(state.shared_arena);
                EXPECT_NE(first_arena.get(), state.shared_arena.get());
                EXPECT_EQ(first_request->get().GetArena(), first_arena.get());
                EXPECT_EQ(first_response->get().GetArena(), first_arena.get());
                EXPECT_EQ(
                    dynamic_cast<const sample::ugrpc::GreetingRequest&>(first_request->get()).name(),
                    request.name()
                );
                EXPECT_EQ(dynamic_cast<const Response&>(first_response->get()).name(), "middleware");
                auto first_arena_copy = first_arena;
                first_arena.reset();
                EXPECT_EQ(state.destroyed, 0);
                EXPECT_EQ(dynamic_cast<const Response&>(first_response->get()).name(), "middleware");
                first_arena_copy.reset();
                EXPECT_EQ(state.destroyed, 1);
                EXPECT_EQ(state.sent_response->get().GetArena(), state.shared_arena.get());
                EXPECT_EQ(dynamic_cast<const Response&>(state.sent_response->get()).name(), "middleware");
                state.shared_arena.reset();
                EXPECT_EQ(state.destroyed, 2);
            }
        }
    }
}

UTEST(GrpcArenaResult, LocalArenaResponseIsOwned) {
    const auto make_result = []() -> ugrpc::server::Result<Response> {
        google::protobuf::Arena arena;
        auto& response = *protobuf::CreateArenaMessage<Response>(arena);
        response.set_name("local");
        return std::move(response);
    };
    auto result = make_result();
    EXPECT_EQ(result.GetResponse().name(), "local");
    EXPECT_EQ(result.GetResponse().GetArena(), nullptr);
    std::optional<ugrpc::server::impl::MessageStorage<Response>> unpacked;
    grpc::Status status;
    ugrpc::server::impl::UnpackResult(std::move(result), unpacked, status);
    ASSERT_TRUE(unpacked);
    EXPECT_EQ(unpacked->Get().name(), "local");
}

UTEST(GrpcArenaResult, LocalArenaLastResponseIsOwned) {
    const auto make_result = []() -> ugrpc::server::StreamingResult<Response> {
        google::protobuf::Arena arena;
        auto& response = *protobuf::CreateArenaMessage<Response>(arena);
        response.set_name("local");
        return std::move(response);
    };
    auto result = make_result();
    EXPECT_EQ(result.GetLastResponse().name(), "local");
    EXPECT_EQ(result.GetLastResponse().GetArena(), nullptr);
    std::optional<ugrpc::server::impl::MessageStorage<Response>> unpacked;
    grpc::Status status;
    ugrpc::server::impl::UnpackResult(std::move(result), unpacked, status);
    ASSERT_TRUE(unpacked);
    EXPECT_EQ(unpacked->Get().name(), "local");
}

UTEST(GrpcArenaResult, BorrowAndMove) {
    using Result = ugrpc::server::Result<Response>;
    google::protobuf::Arena arena;
    auto& response = *protobuf::CreateArenaMessage<Response>(arena);
    response.set_name("arena");
    auto result = Result::BorrowArenaResponse(response);
    EXPECT_EQ(&result.GetResponse(), &response);
    auto moved = std::move(result);
    EXPECT_EQ(&moved.GetResponse(), &response);
    Result assigned(grpc::Status{grpc::StatusCode::INTERNAL, "error"});
    assigned = std::move(moved);
    EXPECT_TRUE(assigned.IsSuccess());
    EXPECT_EQ(&assigned.GetResponse(), &response);
    Result owned(Response{});
    owned = std::move(assigned);
    EXPECT_EQ(&owned.GetResponse(), &response);
    auto&& extracted = owned.ExtractResponse();
    EXPECT_EQ(&extracted, &response);
    EXPECT_EQ(extracted.name(), "arena");
}

UTEST(GrpcArenaResult, StreamingBorrowAndMove) {
    using Result = ugrpc::server::StreamingResult<Response>;
    google::protobuf::Arena arena;
    auto& response = *protobuf::CreateArenaMessage<Response>(arena);
    response.set_name("arena");
    auto result = Result::BorrowArenaResponse(response);
    EXPECT_EQ(&result.GetLastResponse(), &response);
    auto moved = std::move(result);
    EXPECT_EQ(&moved.GetLastResponse(), &response);
    Result assigned(grpc::Status{grpc::StatusCode::INTERNAL, "error"});
    assigned = std::move(moved);
    EXPECT_TRUE(assigned.HasLastResponse());
    EXPECT_TRUE(assigned.IsSuccess());
    EXPECT_EQ(&assigned.GetLastResponse(), &response);
    Result owned(Response{});
    owned = std::move(assigned);
    EXPECT_EQ(&owned.GetLastResponse(), &response);
    auto&& extracted = owned.ExtractLastResponse();
    EXPECT_EQ(&extracted, &response);
    EXPECT_EQ(extracted.name(), "arena");
}

UTEST(GrpcArenaResult, UnpackPreservesIdentity) {
    google::protobuf::Arena arena;
    auto& response = *protobuf::CreateArenaMessage<Response>(arena);
    std::optional<ugrpc::server::impl::MessageStorage<Response>> unpacked;
    grpc::Status status;
    {
        auto result = ugrpc::server::Result<Response>::BorrowArenaResponse(response);
        ugrpc::server::impl::UnpackResult(std::move(result), unpacked, status);
    }
    ASSERT_TRUE(unpacked);
    EXPECT_EQ(&unpacked->Get(), &response);
    EXPECT_TRUE(status.ok());
    {
        auto streaming = ugrpc::server::StreamingResult<Response>::BorrowArenaResponse(response);
        ugrpc::server::impl::UnpackResult(std::move(streaming), unpacked, status);
    }
    ASSERT_TRUE(unpacked);
    EXPECT_EQ(&unpacked->Get(), &response);
    EXPECT_TRUE(status.ok());
}

UTEST(GrpcArenaResult, NonProtobufAndMoveOnly) {
    ugrpc::server::Result<std::string> value(std::string("owned"));
    auto moved_value = std::move(value);
    EXPECT_EQ(moved_value.GetResponse(), "owned");
    ugrpc::server::Result<std::unique_ptr<int>> pointer(std::make_unique<int>(42));
    static_assert(!std::is_copy_constructible_v<decltype(pointer)>);
    auto moved = std::move(pointer);
    EXPECT_EQ(*moved.GetResponse(), 42);
}

UTEST(GrpcArenaResult, UninitializedResponse) {
    struct Proto2Traits {
        using Response = sample::ugrpc::Proto2Response;
    };
    google::protobuf::Arena arena;
    auto& message = *protobuf::CreateArenaMessage<sample::ugrpc::Proto2Response>(arena);
    ugrpc::server::Result<sample::ugrpc::Proto2Response> result(std::move(message));
    grpc::Status status;
    std::optional<ugrpc::server::impl::MessageStorage<sample::ugrpc::Proto2Response>> storage;
    ugrpc::server::impl::UnpackResult(std::move(result), storage, status);
    ugrpc::server::impl::ValidateResponse<Proto2Traits>(storage, status);
    EXPECT_FALSE(storage);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
}

UTEST_MT(ProtobufArenaMigration, ConcurrentMessages, 4) {
#if USERVER_IMPL_HAS_TSAN
    GTEST_SKIP() << "TSan uses a non-migrating scheduler";
#endif

    constexpr int kTaskCount = 16;
    constexpr int kMessageSize = 1024;
    static compiler::ThreadLocal thread_id = [] { return std::this_thread::get_id(); };
    const auto get_thread_id = [] {
        auto scope = thread_id.Use();
        return *scope;
    };

    google::protobuf::Arena shared_arena;
    std::atomic<bool> migrated{false};
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTaskCount);
    for (int task_index = 0; task_index < kTaskCount; ++task_index) {
        tasks.push_back(engine::AsyncNoTracing([&, task_index] {
            google::protobuf::Arena private_arena;
            for (auto* arena : std::array{&private_arena, &shared_arena}) {
                google::protobuf::RepeatedField<int> numbers(arena);
                auto& message = *protobuf::CreateArenaMessage<google::protobuf::ListValue>(*arena);
                auto previous_thread = get_thread_id();
                for (int index = 0; index < kMessageSize; ++index) {
                    numbers.Add(task_index + index);
                    message.add_values()->set_number_value(task_index + index);
                    engine::Yield();
                    const auto current_thread = get_thread_id();
                    if (current_thread != previous_thread) {
                        migrated.store(true, std::memory_order_relaxed);
                    }
                    previous_thread = current_thread;
                }
                for (int index = 0; index < kMessageSize; ++index) {
                    EXPECT_EQ(numbers.Get(index), task_index + index);
                    EXPECT_EQ(message.values(index).number_value(), task_index + index);
                }
                const auto serialized = message.SerializeAsString();
                engine::Yield();
                auto& parsed = *protobuf::CreateArenaMessage<google::protobuf::ListValue>(*arena);
                ASSERT_TRUE(parsed.ParseFromString(serialized));
                EXPECT_EQ(parsed.SerializeAsString(), serialized);
            }
        }));
    }
    engine::WaitAllChecked(tasks);
    EXPECT_TRUE(migrated.load());
}

USERVER_NAMESPACE_END
