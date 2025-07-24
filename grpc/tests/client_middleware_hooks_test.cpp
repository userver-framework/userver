#include <userver/utest/utest.hpp>

#include <userver/engine/sleep.hpp>

#include <tests/client_middleware_base_gmock.hpp>
#include <tests/middlewares_fixture.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceMock : public sample::ugrpc::UnitTestServiceBase {
public:
    MOCK_METHOD(SayHelloResult, SayHello, (CallContext&, sample::ugrpc::GreetingRequest&&), (override));

    MOCK_METHOD(
        ReadManyResult,
        ReadMany,
        (CallContext&, sample::ugrpc::StreamGreetingRequest&&, ReadManyWriter&),
        (override)
    );

    MOCK_METHOD(WriteManyResult, WriteMany, (CallContext&, WriteManyReader&), (override));

    MOCK_METHOD(ChatResult, Chat, (CallContext&, ChatReaderWriter&), (override));
};

class ClientMiddlewaresHooksTest : public tests::MiddlewaresFixture<
                                       tests::client::ClientMiddlewareBaseMock,
                                       ::testing::NiceMock<UnitTestServiceMock>,
                                       sample::ugrpc::UnitTestServiceClient,
                                       /*N=*/1> {
public:
    using CallContext = ugrpc::server::CallContext;

    using Request = sample::ugrpc::GreetingRequest;
    using Response = sample::ugrpc::GreetingResponse;

    using StreamRequest = sample::ugrpc::StreamGreetingRequest;
    using StreamResponse = sample::ugrpc::StreamGreetingResponse;

    using UnaryResult = UnitTestServiceMock::SayHelloResult;
    using ServerStreamingResult = UnitTestServiceMock::ReadManyResult;
    using ClientStreamingResult = UnitTestServiceMock::WriteManyResult;
    using BidirectionalStreamingResult = UnitTestServiceMock::ChatResult;

    using Writer = UnitTestServiceMock::ReadManyWriter;
    using Reader = UnitTestServiceMock::WriteManyReader;
    using ReaderWriter = UnitTestServiceMock::ChatReaderWriter;

protected:
    using UnaryCallback = std::function<UnaryResult(CallContext&, Request&&)>;
    using ServerStreamingCallback = std::function<ServerStreamingResult(CallContext&, StreamRequest&&, Writer&)>;
    using ClientStreamingCallback = std::function<ClientStreamingResult(CallContext&, Reader&)>;
    using BidirectionalStreamingCallback = std::function<BidirectionalStreamingResult(CallContext&, ReaderWriter&)>;

    void SetUnary(UnaryCallback cb) { ON_CALL(Service(), SayHello).WillByDefault(std::move(cb)); }

    void SetServerStreaming(ServerStreamingCallback cb) { ON_CALL(Service(), ReadMany).WillByDefault(std::move(cb)); }

    void SetClientStreaming(ClientStreamingCallback cb) { ON_CALL(Service(), WriteMany).WillByDefault(std::move(cb)); }

    void SetBidirectionalStreaming(BidirectionalStreamingCallback cb) {
        ON_CALL(Service(), Chat).WillByDefault(std::move(cb));
    }

    void SetHappyPathUnary() {
        SetUnary([](CallContext&, Request&& request) -> UnaryResult {
            Response response;
            response.set_name("Hello " + request.name());
            return response;
        });
    }

    void SetHappyPathServerStreaming() {
        SetServerStreaming([](CallContext&, StreamRequest&& request, Writer& writer) -> ServerStreamingResult {
            StreamResponse response;
            response.set_name("Hello again " + request.name());
            for (int i = 0; i < request.number(); ++i) {
                response.set_number(i);
                writer.Write(response);
            }

            return grpc::Status::OK;
        });
    }

    void SetHappyPathClientStreaming() {
        SetClientStreaming([](CallContext&, Reader& reader) -> ClientStreamingResult {
            StreamRequest request;

            int count = 0;
            while (reader.Read(request)) {
                ++count;
            }

            StreamResponse response;
            response.set_name("Hello");
            response.set_number(count);
            return response;
        });
    }

    void SetHappyPathBidirectionalStreaming() {
        SetBidirectionalStreaming([](CallContext&, ReaderWriter& stream) -> BidirectionalStreamingResult {
            StreamRequest request;
            StreamResponse response;

            int count = 0;
            while (stream.Read(request)) {
                ++count;
                response.set_number(count);
                response.set_name("Hello " + request.name());
                stream.Write(response);
            }
            return grpc::Status::OK;
        });
    }

    auto GetMetric(std::string_view name, std::vector<utils::statistics::Label> labels = {}) {
        return GetStatistics("grpc.client.total", labels).SingleMetric(std::string{name}, labels).AsRate();
    }
};

}  // namespace

UTEST_F(ClientMiddlewaresHooksTest, HappyPathUnary) {
    SetHappyPathUnary();

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    Request request;
    request.set_name("userver");
    auto response = Client().SayHello(request);

    EXPECT_EQ(response.name(), "Hello userver");
}

UTEST_F(ClientMiddlewaresHooksTest, HappyPathClientStreaming) {
    SetHappyPathClientStreaming();

    constexpr std::size_t kMessages{3};

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(kMessages);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    StreamRequest request;
    request.set_name("userver");
    auto stream = Client().WriteMany();

    for (std::size_t message{1}; message <= kMessages; ++message) {
        EXPECT_TRUE(stream.Write(request));
    }
    const auto response = stream.Finish();

    EXPECT_EQ(response.name(), "Hello");
    EXPECT_EQ(response.number(), kMessages);
}

UTEST_F(ClientMiddlewaresHooksTest, HappyPathServerStreaming) {
    SetHappyPathServerStreaming();

    constexpr std::size_t kMessages{3};

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(kMessages);
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    StreamRequest request;
    request.set_name("userver");
    request.set_number(kMessages);
    auto stream = Client().ReadMany(request);

    StreamResponse response;
    std::size_t message{0};
    while (stream.Read(response)) {
        EXPECT_EQ(response.number(), message);

        message += 1;
    }
    EXPECT_EQ(message, kMessages);
}

UTEST_F(ClientMiddlewaresHooksTest, HappyPathBidirectionalStreaming) {
    SetHappyPathBidirectionalStreaming();

    constexpr std::size_t kMessages{3};

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(kMessages);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(kMessages);
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    auto stream = Client().Chat();

    StreamRequest request;
    StreamResponse response;
    // NOLINTBEGIN(clang-analyzer-optin.cplusplus.UninitializedObject)
    for (std::size_t message{1}; message <= kMessages; ++message) {
        request.set_number(message);

        stream.WriteAndCheck(request);

        EXPECT_TRUE(stream.Read(response));
        EXPECT_EQ(response.number(), message);
    }
    // NOLINTEND(clang-analyzer-optin.cplusplus.UninitializedObject)
    EXPECT_TRUE(stream.WritesDone());
    EXPECT_FALSE(stream.Read(response));
}

UTEST_F(ClientMiddlewaresHooksTest, HappyPathDetailedUnary) {
    SetHappyPathUnary();

    ::testing::MockFunction<void(std::string_view checkpoint_name)> checkpoint;
    {
        const ::testing::InSequence s;

        // Pre* called after request created
        EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
        EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
        EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);
        EXPECT_CALL(Middleware(0), PostFinish).Times(0);
        EXPECT_CALL(checkpoint, Call("AfterCallStart"));

        // Post* called after Finish
        EXPECT_CALL(Middleware(0), PreStartCall).Times(0);
        EXPECT_CALL(Middleware(0), PreSendMessage).Times(0);
        EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);
        EXPECT_CALL(Middleware(0), PostFinish).Times(1);
        EXPECT_CALL(checkpoint, Call("AfterCallDone"));
    }

    Request request;
    request.set_name("userver");

    // Pre* called after request created
    auto future = Client().AsyncSayHello(request);
    checkpoint.Call("AfterCallStart");

    const auto status = future.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
    EXPECT_EQ(status, engine::FutureStatus::kReady);

    const auto response = future.Get();
    EXPECT_EQ(response.name(), "Hello userver");
    checkpoint.Call("AfterCallDone");
}

UTEST_F(ClientMiddlewaresHooksTest, HappyPathDetailedClientStreaming) {
    SetHappyPathClientStreaming();

    constexpr std::size_t kMessages{3};

    ::testing::MockFunction<void(std::string_view checkpoint_name)> checkpoint;
    {
        const ::testing::InSequence s;

        // Pre* called on stream init
        EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
        EXPECT_CALL(checkpoint, Call("AfterCallStart"));

        // PreSend called on each Write
        for (std::size_t i{0}; i < kMessages; ++i) {
            EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
            EXPECT_CALL(checkpoint, Call("AfterWrite"));
        }

        // Post* called after Finish
        EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);
        EXPECT_CALL(Middleware(0), PostFinish).Times(1);
        EXPECT_CALL(checkpoint, Call("AfterCallFinish"));
    }

    StreamRequest request;
    request.set_name("userver");

    auto stream = Client().WriteMany();
    checkpoint.Call("AfterCallStart");

    for (std::size_t message{1}; message <= kMessages; ++message) {
        EXPECT_TRUE(stream.Write(request));
        checkpoint.Call("AfterWrite");
    }

    const auto response = stream.Finish();
    EXPECT_EQ(response.name(), "Hello");
    EXPECT_EQ(response.number(), kMessages);
    checkpoint.Call("AfterCallFinish");
}

UTEST_F(ClientMiddlewaresHooksTest, HappyPathDetailedServerStreaming) {
    SetHappyPathServerStreaming();

    constexpr std::size_t kMessages{3};

    ::testing::MockFunction<void(std::string_view checkpoint_name)> checkpoint;
    {
        const ::testing::InSequence s;

        // Pre* called on stream init
        EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
        EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
        EXPECT_CALL(checkpoint, Call("AfterCallStart"));

        // PostRecv called on each Read
        for (std::size_t i{0}; i < kMessages; ++i) {
            EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);
            EXPECT_CALL(checkpoint, Call("AfterRead"));
        }

        // PostFinish called after Read from completed rpc
        EXPECT_CALL(Middleware(0), PostFinish).Times(1);
        EXPECT_CALL(checkpoint, Call("AfterFinalRead"));
    }

    StreamRequest request;
    request.set_name("userver");
    request.set_number(kMessages);
    StreamResponse response;

    auto stream = Client().ReadMany(request);
    checkpoint.Call("AfterCallStart");

    for (std::size_t message{0}; message < kMessages; ++message) {
        EXPECT_TRUE(stream.Read(response));
        checkpoint.Call("AfterRead");

        EXPECT_EQ(response.number(), message);
    }

    EXPECT_FALSE(stream.Read(response));
    checkpoint.Call("AfterFinalRead");
}

UTEST_F(ClientMiddlewaresHooksTest, HappyPathDetailedBidirectionalStreaming) {
    SetHappyPathBidirectionalStreaming();

    constexpr std::size_t kMessages{3};

    ::testing::MockFunction<void(std::string_view checkpoint_name)> checkpoint;
    {
        const ::testing::InSequence s;

        // PreStart called on stream init
        EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
        EXPECT_CALL(checkpoint, Call("AfterCallStart"));

        // PreSend called on each Write
        // PreRecv called on each Read
        for (std::size_t i{0}; i < kMessages; ++i) {
            EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
            EXPECT_CALL(checkpoint, Call("AfterWrite"));

            EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);
            EXPECT_CALL(checkpoint, Call("AfterRead"));
        }

        // PostFinish called after Read from completed rpc
        EXPECT_CALL(Middleware(0), PostFinish).Times(1);
        EXPECT_CALL(checkpoint, Call("AfterFinalRead"));
    }

    StreamRequest request;
    StreamResponse response;

    auto stream = Client().Chat();
    checkpoint.Call("AfterCallStart");

    // NOLINTBEGIN(clang-analyzer-optin.cplusplus.UninitializedObject)
    for (std::size_t message{1}; message <= kMessages; ++message) {
        request.set_number(message);

        stream.WriteAndCheck(request);
        checkpoint.Call("AfterWrite");

        EXPECT_TRUE(stream.Read(response));
        checkpoint.Call("AfterRead");

        EXPECT_EQ(response.number(), message);
    }
    // NOLINTEND(clang-analyzer-optin.cplusplus.UninitializedObject)

    EXPECT_TRUE(stream.WritesDone());
    EXPECT_FALSE(stream.Read(response));

    checkpoint.Call("AfterFinalRead");
}

UTEST_F(ClientMiddlewaresHooksTest, MiddlewareExceptionUnaryPreStart) {
    SetHappyPathUnary();

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    ON_CALL(Middleware(0), PreStartCall).WillByDefault([](const ugrpc::client::MiddlewareCallContext&) {
        throw std::runtime_error{"mock error"};
    });

    Request request;
    request.set_name("userver");
    UEXPECT_THROW(auto future = Client().AsyncSayHello(request), std::runtime_error);
}

UTEST_F(ClientMiddlewaresHooksTest, MiddlewareExceptionUnaryPreSend) {
    SetHappyPathUnary();

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    ON_CALL(Middleware(0), PreSendMessage)
        .WillByDefault([](const ugrpc::client::MiddlewareCallContext&, const google::protobuf::Message&) {
            throw std::runtime_error{"mock error"};
        });

    Request request;
    request.set_name("userver");
    UEXPECT_THROW(auto future = Client().AsyncSayHello(request), std::runtime_error);
}

UTEST_F(ClientMiddlewaresHooksTest, MiddlewareExceptionUnaryPostRecv) {
    SetHappyPathUnary();

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    ON_CALL(Middleware(0), PostRecvMessage)
        .WillByDefault([](const ugrpc::client::MiddlewareCallContext&, const google::protobuf::Message&) {
            throw std::runtime_error{"mock error"};
        });

    Request request;
    request.set_name("userver");
    std::optional<sample::ugrpc::UnitTestServiceClient::SayHelloResponseFuture> future;
    UEXPECT_NO_THROW(future.emplace(Client().AsyncSayHello(request)));

    UEXPECT_THROW(future->Get(), std::runtime_error);
}

UTEST_F(ClientMiddlewaresHooksTest, MiddlewareExceptionUnaryPostFinish) {
    SetHappyPathUnary();

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    ON_CALL(Middleware(0), PostFinish)
        .WillByDefault([](const ugrpc::client::MiddlewareCallContext&, const grpc::Status&) {
            throw std::runtime_error{"mock error"};
        });

    Request request;
    request.set_name("userver");
    std::optional<sample::ugrpc::UnitTestServiceClient::SayHelloResponseFuture> future;
    UEXPECT_NO_THROW(future.emplace(Client().AsyncSayHello(request)));

    UEXPECT_THROW(future->Get(), std::runtime_error);
}

UTEST_F(ClientMiddlewaresHooksTest, MiddlewareExceptionClientStreaming) {
    SetHappyPathClientStreaming();

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    ON_CALL(Middleware(0), PreStartCall).WillByDefault([](const ugrpc::client::MiddlewareCallContext&) {
        throw std::runtime_error{"mock error"};
    });

    UEXPECT_THROW(auto future = Client().WriteMany(), std::runtime_error);
}

UTEST_F(ClientMiddlewaresHooksTest, MiddlewareExceptionServerStreaming) {
    SetHappyPathServerStreaming();

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    ON_CALL(Middleware(0), PreStartCall).WillByDefault([](const ugrpc::client::MiddlewareCallContext&) {
        throw std::runtime_error{"mock error"};
    });

    StreamRequest request;
    request.set_name("userver");
    UEXPECT_THROW(auto future = Client().ReadMany(request), std::runtime_error);
}

UTEST_F(ClientMiddlewaresHooksTest, MiddlewareExceptionBidirectionalStreaming) {
    SetHappyPathBidirectionalStreaming();

    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    ON_CALL(Middleware(0), PreStartCall).WillByDefault([](const ugrpc::client::MiddlewareCallContext&) {
        throw std::runtime_error{"mock error"};
    });

    UEXPECT_THROW(auto future = Client().Chat(), std::runtime_error);
}

UTEST_F(ClientMiddlewaresHooksTest, ExceptionWhenCancelledUnary) {
    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);  // skipped, because no response message.
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    SetUnary([](CallContext&, Request&&) -> UnaryResult {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);

        return Response{};
    });

    {
        Request request;
        request.set_name("userver");
        const auto future = Client().AsyncSayHello(request);

        engine::current_task::GetCancellationToken().RequestCancel();

        // The destructor of `future` will cancel the RPC and await grpcpp cleanup (and don't run middlewares).
        // Cancellation should not lead to a crash.
    }
}

UTEST_F(ClientMiddlewaresHooksTest, BadStatusUnary) {
    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);  // skipped, because no response message
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    SetUnary([](CallContext&, Request&&) -> UnaryResult {
        return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "mocked status"};
    });

    Request request;
    request.set_name("userver");
    UEXPECT_THROW(auto response = Client().SayHello(request), ugrpc::client::InvalidArgumentError);
}

UTEST_F(ClientMiddlewaresHooksTest, BadStatusClientStreaming) {
    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);  // skipped, because no response message
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    engine::SingleUseEvent wait_write;
    SetClientStreaming([&wait_write](CallContext&, Reader&) -> ClientStreamingResult {
        wait_write.Wait();

        return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "mocked status"};
    });

    auto stream = Client().WriteMany();

    StreamRequest request;
    request.set_name("userver");
    UASSERT_NO_THROW(stream.WriteAndCheck(request));
    wait_write.Send();

    UEXPECT_THROW(auto response = stream.Finish(), ugrpc::client::InvalidArgumentError);
}

UTEST_F(ClientMiddlewaresHooksTest, Abandoned) {
    SetHappyPathBidirectionalStreaming();
    SetHappyPathClientStreaming();
    SetHappyPathServerStreaming();
    SetHappyPathUnary();

    // Five streams were created.
    EXPECT_CALL(Middleware(0), PreStartCall).Times(5);
    // WriteAndCheck + ReadMany + AsyncSayHello
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(3);
    // Skipped, because no response messages.
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);
    // We don't run middlewares in a destructors of RPC.
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    ON_CALL(Middleware(0), PostFinish)
        .WillByDefault([](const ugrpc::client::MiddlewareCallContext&, const grpc::Status&) {
            throw std::runtime_error{"mock error"};
        });

    auto check_metrics = [this](std::size_t abandoned) {
        EXPECT_EQ(GetMetric("abandoned-error"), abandoned);
        EXPECT_FALSE(GetStatistics("grpc.client.total", {{"grpc_code", "CANCELLED"}}).SingleMetricOptional("status"));

        EXPECT_EQ(GetMetric("cancelled"), 0);
        EXPECT_EQ(GetMetric("status", {{"grpc_code", "OK"}}), 0);
        EXPECT_EQ(GetMetric("status", {{"grpc_code", "UNKNOWN"}}), 0);
    };

    {
        EXPECT_FALSE(GetStatistics("grpc.client.total").SingleMetricOptional("abandoned-error"));
        EXPECT_FALSE(GetStatistics("grpc.client.total", {{"grpc_code", "CANCELLED"}}).SingleMetricOptional("status"));
        {
            auto stream = Client().WriteMany();
            StreamRequest request;
            UASSERT_NO_THROW(stream.WriteAndCheck(request));
        }
        UASSERT_NO_THROW(const auto stream = Client().WriteMany());
    }
    check_metrics(2);
    {
        StreamRequest request;
        UEXPECT_NO_THROW(const auto stream = Client().ReadMany(request));
    }
    check_metrics(3);
    {
        Request request;
        UEXPECT_NO_THROW(auto future = Client().AsyncSayHello(request));
    }
    check_metrics(4);
    {
        auto stream = Client().Chat();
        StreamResponse response;
        UEXPECT_NO_THROW(const auto future = stream.ReadAsync(response));
    }
    check_metrics(5);
}

UTEST_F(ClientMiddlewaresHooksTest, BadStatusServerStreaming) {
    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);  // Second call is skipped, because no response message
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    // Fail after first Write (on server side)
    engine::SingleUseEvent wait_read;
    SetServerStreaming([&wait_read](CallContext&, StreamRequest&& request, Writer& writer) -> ServerStreamingResult {
        StreamResponse response;
        response.set_name("Hello again " + request.name());
        writer.Write(response);

        wait_read.Wait();

        return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "mocked status"};
    });

    StreamRequest request;
    request.set_name("userver");
    request.set_number(3);
    StreamResponse response;

    auto stream = Client().ReadMany(request);

    EXPECT_TRUE(stream.Read(response));
    wait_read.Send();

    UEXPECT_THROW([[maybe_unused]] auto ok = stream.Read(response), ugrpc::client::InvalidArgumentError);
}

UTEST_F(ClientMiddlewaresHooksTest, BadStatusBidirectionalStreaming) {
    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(1);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(1);  // Second call is skipped, because no response message
    EXPECT_CALL(Middleware(0), PostFinish).Times(1);

    // Fail after first Write (on server side)
    engine::SingleUseEvent wait_read;
    SetBidirectionalStreaming([&wait_read](CallContext&, ReaderWriter& stream) -> BidirectionalStreamingResult {
        StreamRequest request;
        EXPECT_TRUE(stream.Read(request));

        StreamResponse response;
        response.set_number(0);
        response.set_name("Hello " + request.name());
        stream.Write(response);

        wait_read.Wait();

        return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "mocked status"};
    });

    StreamRequest request;
    request.set_name("userver");
    request.set_number(3);
    StreamResponse response;

    auto stream = Client().Chat();

    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    UEXPECT_NO_THROW(stream.WriteAndCheck(request));

    EXPECT_TRUE(stream.Read(response));
    wait_read.Send();

    UEXPECT_THROW([[maybe_unused]] auto ok = stream.Read(response), ugrpc::client::InvalidArgumentError);
}

UTEST_F(ClientMiddlewaresHooksTest, ThrowInDestructorBidirectional) {
    SetBidirectionalStreaming([](CallContext&, ReaderWriter&) -> BidirectionalStreamingResult {
        return grpc::Status{};
    });
    EXPECT_CALL(Middleware(0), PreStartCall).Times(1);
    EXPECT_CALL(Middleware(0), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(0), PostRecvMessage).Times(0);
    // We don't run middlewares in a destructors of RPC.
    EXPECT_CALL(Middleware(0), PostFinish).Times(0);

    ON_CALL(Middleware(0), PostFinish)
        .WillByDefault([](const ugrpc::client::MiddlewareCallContext&, const grpc::Status&) {
            throw std::runtime_error{"mock error"};
        });

    auto stream = Client().Chat();
    StreamResponse response;
    UEXPECT_NO_THROW(const auto future = stream.ReadAsync(response));
}

USERVER_NAMESPACE_END
