#include <userver/utest/utest.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <google/protobuf/message.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/status.h>

#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>

#include <userver/ugrpc/client/call_options.hpp>
#include <userver/ugrpc/client/completion_status.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/server/generic_service_base.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <tests/client_middleware_base_gmock.hpp>
#include <tests/middlewares_fixture.hpp>
#include <tests/server_middleware_base_gmock.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <tests/unit_test_service_gmock.hpp>
#include <tests/unparsable_response_service.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Request = sample::ugrpc::GreetingRequest;
using Response = sample::ugrpc::GreetingResponse;
using StreamRequest = sample::ugrpc::StreamGreetingRequest;
using StreamResponse = sample::ugrpc::StreamGreetingResponse;

const std::string kRequestName = "userver";
constexpr std::size_t kResponseNameSize = 1000;
constexpr std::size_t kFailedAttempts = 2;
constexpr std::size_t kAttempts = kFailedAttempts + 1;
constexpr std::size_t kStreamMessages = 3;
constexpr std::size_t kBidirectionalMessages = 500;
constexpr std::size_t kStreamNameSizeStep = 100;

Request MakeRequest() {
    Request request;
    request.set_name(kRequestName);
    return request;
}

Response MakeResponse() {
    Response response;
    response.set_name(std::string(kResponseNameSize, 'r'));
    return response;
}

std::string MakeStreamName(std::size_t message_index) {
    return std::string((message_index + 1) * kStreamNameSizeStep, 's');
}

StreamRequest MakeStreamRequest(std::size_t message_index) {
    StreamRequest request;
    request.set_name(MakeStreamName(message_index));
    return request;
}

StreamResponse MakeStreamResponse(std::size_t message_index) {
    StreamResponse response;
    response.set_name(MakeStreamName(message_index));
    response.set_number(static_cast<int>(message_index));
    return response;
}

struct ObservedClientSizes {
    std::vector<std::optional<std::size_t>> request_on_send;
    std::optional<std::size_t> request_on_recv;
    std::vector<std::optional<std::size_t>> response_on_recv;
    std::optional<std::size_t> request_on_finish;
    std::optional<std::size_t> response_on_finish;
};

using ClientMessageSizeTestBase = tests::MiddlewaresFixture<
    tests::client::ClientMiddlewareBaseMock,
    ::testing::NiceMock<tests::UnitTestServiceGmock>,
    sample::ugrpc::UnitTestServiceClient,
    /*N=*/1>;

class ClientMessageSizeTest : public ClientMessageSizeTestBase {
protected:
    void SetResponse(Response response) {
        ON_CALL(Service(), SayHello)
            .WillByDefault([response = std::move(response)](ugrpc::server::CallContext&, Request&&) {
                auto response_copy = response;
                return tests::UnitTestServiceGmock::SayHelloResult{std::move(response_copy)};
            });
    }

    void SetFailedAttemptsBeforeResponse(std::size_t failed_attempts, Response response) {
        ON_CALL(Service(), SayHello)
            .WillByDefault([failed_attempts,
                            response = std::move(response),
                            calls = std::size_t{0}](ugrpc::server::CallContext&, Request&&) mutable {
                if (calls++ < failed_attempts) {
                    return tests::UnitTestServiceGmock::SayHelloResult{
                        grpc::Status{grpc::StatusCode::UNAVAILABLE, "retry me"}
                    };
                }
                auto response_copy = response;
                return tests::UnitTestServiceGmock::SayHelloResult{std::move(response_copy)};
            });
    }

    ObservedClientSizes& ObserveSizes() {
        ON_CALL(Middleware(), PreSendMessage)
            .WillByDefault([this](ugrpc::client::MiddlewareCallContext& context, const google::protobuf::Message&) {
                observed_.request_on_send.push_back(context.GetLatestRequestSizeBytes());
            });
        ON_CALL(Middleware(), PostRecvMessage)
            .WillByDefault([this](ugrpc::client::MiddlewareCallContext& context, const google::protobuf::Message&) {
                observed_.request_on_recv = context.GetLatestRequestSizeBytes();
                observed_.response_on_recv.push_back(context.GetLatestResponseSizeBytes());
            });
        ON_CALL(Middleware(), PostFinish)
            .WillByDefault(
                [this](ugrpc::client::MiddlewareCallContext& context, const ugrpc::client::CompletionStatus&) {
                    observed_.request_on_finish = context.GetLatestRequestSizeBytes();
                    observed_.response_on_finish = context.GetLatestResponseSizeBytes();
                }
            );
        return observed_;
    }

    ObservedClientSizes CallAndObserveSizes(ugrpc::client::CallOptions options = {}) {
        ObserveSizes();
        UEXPECT_NO_THROW(Client().SayHello(MakeRequest(), std::move(options)));
        return observed_;
    }

private:
    ObservedClientSizes observed_;
};

class TrailersOnlyOkGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext&, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request_bytes;
        EXPECT_TRUE(stream.Read(request_bytes));
        return grpc::Status::OK;
    }
};

using NiceClientMiddlewareMock = ::testing::NiceMock<tests::client::ClientMiddlewareBaseMock>;

ugrpc::tests::ServiceConfigs MakeConfigsWithClientMiddleware(std::shared_ptr<NiceClientMiddlewareMock> middleware) {
    ugrpc::tests::ServiceConfigs configs;
    configs.client_middlewares = {std::move(middleware)};
    return configs;
}

struct ObservedGenericResponse {
    std::size_t post_recv_calls{0};
    std::vector<std::optional<std::size_t>> response_on_start;
    std::optional<std::size_t> response_on_recv;
    std::vector<std::optional<std::size_t>> response_on_finish;
};

template <typename GenericService>
class GenericResponseSizeTest
    : public ugrpc::tests::ServiceWithClientFixture<GenericService, sample::ugrpc::UnitTestServiceClient> {
public:
    GenericResponseSizeTest()
        : GenericResponseSizeTest(std::make_shared<NiceClientMiddlewareMock>())
    {}

protected:
    ObservedGenericResponse& ObserveResponse() {
        ON_CALL(*middleware_, PreStartCall).WillByDefault([this](ugrpc::client::MiddlewareCallContext& context) {
            observed_.response_on_start.push_back(context.GetLatestResponseSizeBytes());
        });
        ON_CALL(*middleware_, PostRecvMessage)
            .WillByDefault([this](ugrpc::client::MiddlewareCallContext& context, const google::protobuf::Message&) {
                ++observed_.post_recv_calls;
                observed_.response_on_recv = context.GetLatestResponseSizeBytes();
            });
        ON_CALL(*middleware_, PostFinish)
            .WillByDefault(
                [this](ugrpc::client::MiddlewareCallContext& context, const ugrpc::client::CompletionStatus&) {
                    observed_.response_on_finish.push_back(context.GetLatestResponseSizeBytes());
                }
            );
        return observed_;
    }

private:
    explicit GenericResponseSizeTest(std::shared_ptr<NiceClientMiddlewareMock> middleware)
        : ugrpc::tests::ServiceWithClientFixture<
              GenericService,
              sample::ugrpc::UnitTestServiceClient>(MakeConfigsWithClientMiddleware(middleware)),
          middleware_(std::move(middleware))
    {}

    std::shared_ptr<NiceClientMiddlewareMock> middleware_;
    ObservedGenericResponse observed_;
};

using TrailersOnlyOkSizeTest = GenericResponseSizeTest<TrailersOnlyOkGenericService>;
using UnparsableResponseSizeTest = GenericResponseSizeTest<tests::UnparsableResponseGenericService>;
using UnparsableThenTrailersOnlySizeTest = GenericResponseSizeTest<tests::UnparsableThenTrailersOnlyGenericService>;

struct ObservedServerSizes {
    std::vector<std::optional<std::size_t>> request_on_recv;
    std::optional<std::size_t> request_on_finish;
    std::optional<std::size_t> response_on_finish;
};

using ServerMessageSizeTestBase = tests::MiddlewaresFixture<
    tests::server::ServerMiddlewareBaseMock,
    ::testing::NiceMock<tests::UnitTestServiceGmock>,
    sample::ugrpc::UnitTestServiceClient,
    /*N=*/1>;

class ServerMessageSizeTest : public ServerMessageSizeTestBase {
protected:
    ~ServerMessageSizeTest() override { GetServer().StopServing(); }

    ObservedServerSizes& ObserveSizes() {
        ON_CALL(Middleware(), PostRecvMessage)
            .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
                observed_.request_on_recv.push_back(context.GetLatestRequestSizeBytes());
            });
        ON_CALL(Middleware(), OnCallFinish)
            .WillByDefault([this](ugrpc::server::MiddlewareCallContext& context, const std::optional<grpc::Status>&) {
                observed_.request_on_finish = context.GetLatestRequestSizeBytes();
                observed_.response_on_finish = context.GetLatestResponseSizeBytes();
                call_finished_.Send();
            });
        return observed_;
    }

    void WaitForCallFinish() { ASSERT_TRUE(call_finished_.WaitForEventFor(utest::kMaxTestWaitTime)); }

private:
    ObservedServerSizes observed_;
    engine::SingleConsumerEvent call_finished_;
};

}  // namespace

UTEST_F(ClientMessageSizeTest, RequestSizeIsKnownBeforeSend) {
    SetResponse(MakeResponse());

    const auto observed = CallAndObserveSizes();

    EXPECT_THAT(observed.request_on_send, ::testing::ElementsAre(MakeRequest().ByteSizeLong()));
}

UTEST_F(ClientMessageSizeTest, RequestSizeIsKeptUntilResponseIsReceived) {
    SetResponse(MakeResponse());

    const auto observed = CallAndObserveSizes();

    EXPECT_EQ(observed.request_on_recv, MakeRequest().ByteSizeLong());
}

UTEST_F(ClientMessageSizeTest, ResponseSizeIsKnownBeforeParsing) {
    SetResponse(MakeResponse());

    const auto observed = CallAndObserveSizes();

    EXPECT_THAT(observed.response_on_recv, ::testing::ElementsAre(MakeResponse().ByteSizeLong()));
}

UTEST_F(ClientMessageSizeTest, EmptyResponseHasZeroSize) {
    SetResponse(Response{});

    const auto observed = CallAndObserveSizes();

    EXPECT_THAT(observed.response_on_recv, ::testing::ElementsAre(std::size_t{0}));
}

UTEST_F(TrailersOnlyOkSizeTest, ResponseWithoutMessageHasZeroSize) {
    const auto& observed = ObserveResponse();

    UEXPECT_NO_THROW(GetClient().SayHello(MakeRequest()));

    EXPECT_EQ(observed.post_recv_calls, std::size_t{1});
    EXPECT_EQ(observed.response_on_recv, std::size_t{0});
}

UTEST_F(UnparsableResponseSizeTest, ResponseSizeIsKnownEvenIfResponseWasNotParsed) {
    const auto& observed = ObserveResponse();

    UEXPECT_THROW(GetClient().SayHello(MakeRequest()), ugrpc::client::RpcInterruptedError);

    EXPECT_EQ(observed.post_recv_calls, std::size_t{0});
    EXPECT_THAT(observed.response_on_finish, ::testing::ElementsAre(tests::kUnparsableResponseSize));
}

UTEST_F(UnparsableThenTrailersOnlySizeTest, ResponseSizeIsNotKeptForTheNextAttempt) {
    const auto& observed = ObserveResponse();
    ugrpc::client::CallOptions options;
    options.SetAttempts(2);

    UEXPECT_NO_THROW(GetClient().SayHello(MakeRequest(), std::move(options)));

    EXPECT_THAT(observed.response_on_start, ::testing::ElementsAre(std::nullopt, std::nullopt));
    EXPECT_THAT(observed.response_on_finish, ::testing::ElementsAre(tests::kUnparsableResponseSize, std::size_t{0}));
}

UTEST_F(ClientMessageSizeTest, SizesAreKnownInPostFinish) {
    SetResponse(MakeResponse());

    const auto observed = CallAndObserveSizes();

    EXPECT_EQ(observed.request_on_finish, MakeRequest().ByteSizeLong());
    EXPECT_EQ(observed.response_on_finish, MakeResponse().ByteSizeLong());
}

UTEST_F(ClientMessageSizeTest, EveryRetryAttemptReportsTheSameRequestSize) {
    SetFailedAttemptsBeforeResponse(kFailedAttempts, MakeResponse());
    ugrpc::client::CallOptions options;
    options.SetAttempts(static_cast<int>(kAttempts));

    const auto observed = CallAndObserveSizes(std::move(options));

    const std::vector<std::optional<std::size_t>> expected_request_sizes(kAttempts, MakeRequest().ByteSizeLong());
    EXPECT_EQ(observed.request_on_send, expected_request_sizes);
    EXPECT_THAT(observed.response_on_recv, ::testing::ElementsAre(MakeResponse().ByteSizeLong()));
}

UTEST_F(ClientMessageSizeTest, FailedCallHasRequestSizeButNoResponseSize) {
    ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, Request&&) {
        return tests::UnitTestServiceGmock::SayHelloResult{grpc::Status{grpc::StatusCode::UNAVAILABLE, "no response"}};
    });
    auto& observed = ObserveSizes();
    ugrpc::client::CallOptions options;
    options.SetAttempts(1);

    UEXPECT_THROW(Client().SayHello(MakeRequest(), std::move(options)), ugrpc::client::UnavailableError);

    EXPECT_EQ(observed.request_on_finish, MakeRequest().ByteSizeLong());
    EXPECT_EQ(observed.response_on_finish, std::nullopt);
}

UTEST_F(ServerMessageSizeTest, UnaryRequestSizeIsKnownBeforeParsing) {
    ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, Request&&) {
        return tests::UnitTestServiceGmock::SayHelloResult{MakeResponse()};
    });
    const auto& observed = ObserveSizes();

    UEXPECT_NO_THROW(Client().SayHello(MakeRequest()));
    WaitForCallFinish();

    EXPECT_THAT(observed.request_on_recv, ::testing::ElementsAre(MakeRequest().ByteSizeLong()));
}

UTEST_F(ServerMessageSizeTest, UnaryResponseSizeIsKnownOnCallFinish) {
    ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, Request&&) {
        return tests::UnitTestServiceGmock::SayHelloResult{MakeResponse()};
    });
    const auto& observed = ObserveSizes();

    UEXPECT_NO_THROW(Client().SayHello(MakeRequest()));
    WaitForCallFinish();

    EXPECT_EQ(observed.request_on_finish, MakeRequest().ByteSizeLong());
    EXPECT_EQ(observed.response_on_finish, MakeResponse().ByteSizeLong());
}

UTEST_F(ServerMessageSizeTest, UnaryResponseSizeIsNotKnownBeforeTheResponseIsSerialized) {
    ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, Request&&) {
        return tests::UnitTestServiceGmock::SayHelloResult{MakeResponse()};
    });
    constexpr std::size_t kUnsetSentinel = 123456789;
    std::optional<std::size_t> size_on_send_message{kUnsetSentinel};
    std::optional<std::size_t> size_on_send_status{kUnsetSentinel};
    ON_CALL(Middleware(), PreSendMessage)
        .WillByDefault([&](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            size_on_send_message = context.GetLatestResponseSizeBytes();
        });
    ON_CALL(Middleware(), PreSendStatus)
        .WillByDefault([&](ugrpc::server::MiddlewareCallContext& context, grpc::Status&) {
            size_on_send_status = context.GetLatestResponseSizeBytes();
        });
    const auto& observed = ObserveSizes();

    UEXPECT_NO_THROW(Client().SayHello(MakeRequest()));
    WaitForCallFinish();

    EXPECT_EQ(size_on_send_message, std::nullopt);
    EXPECT_EQ(size_on_send_status, std::nullopt);
    EXPECT_EQ(observed.response_on_finish, MakeResponse().ByteSizeLong());
}

UTEST_F(ServerMessageSizeTest, EmptyUnaryResponseHasZeroSize) {
    ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, Request&&) {
        return tests::UnitTestServiceGmock::SayHelloResult{Response{}};
    });
    const auto& observed = ObserveSizes();

    UEXPECT_NO_THROW(Client().SayHello(MakeRequest()));
    WaitForCallFinish();

    EXPECT_EQ(observed.response_on_finish, std::size_t{0});
}

UTEST_F(ServerMessageSizeTest, ErrorStatusHasNoResponseSize) {
    ON_CALL(Service(), SayHello).WillByDefault([](ugrpc::server::CallContext&, Request&&) {
        return tests::UnitTestServiceGmock::SayHelloResult{grpc::Status{grpc::StatusCode::NOT_FOUND, "no response"}};
    });
    const auto& observed = ObserveSizes();

    UEXPECT_THROW(Client().SayHello(MakeRequest()), ugrpc::client::NotFoundError);
    WaitForCallFinish();

    EXPECT_EQ(observed.request_on_finish, MakeRequest().ByteSizeLong());
    EXPECT_EQ(observed.response_on_finish, std::nullopt);
}

UTEST_F(ServerMessageSizeTest, ClientStreamingReportsSizeOfEveryRequestMessage) {
    ON_CALL(Service(), WriteMany)
        .WillByDefault([](ugrpc::server::CallContext&, tests::UnitTestServiceGmock::WriteManyReader& reader) {
            StreamRequest request;
            while (reader.Read(request)) {
            }
            return tests::UnitTestServiceGmock::WriteManyResult{MakeStreamResponse(0)};
        });
    const auto& observed = ObserveSizes();

    auto stream = Client().WriteMany();
    std::vector<std::optional<std::size_t>> expected_request_sizes;
    for (std::size_t message = 0; message < kStreamMessages; ++message) {
        const auto request = MakeStreamRequest(message);
        expected_request_sizes.emplace_back(request.ByteSizeLong());
        EXPECT_TRUE(stream.Write(request));
    }
    UEXPECT_NO_THROW(std::ignore = stream.Finish());
    WaitForCallFinish();

    EXPECT_EQ(observed.request_on_recv, expected_request_sizes);
    EXPECT_EQ(observed.response_on_finish, MakeStreamResponse(0).ByteSizeLong());
}

UTEST_F(ServerMessageSizeTest, ServerStreamingReportsSizeOfTheLastResponseMessage) {
    ON_CALL(Service(), ReadMany)
        .WillByDefault([](ugrpc::server::CallContext&,
                          StreamRequest&&,
                          tests::UnitTestServiceGmock::ReadManyWriter& writer) {
            for (std::size_t message = 0; message < kStreamMessages; ++message) {
                writer.Write(MakeStreamResponse(message));
            }
            return tests::UnitTestServiceGmock::ReadManyResult{grpc::Status::OK};
        });
    const auto& observed = ObserveSizes();

    auto stream = Client().ReadMany(MakeStreamRequest(0));
    StreamResponse response;
    while (stream.Read(response)) {
    }
    WaitForCallFinish();

    EXPECT_EQ(observed.request_on_finish, MakeStreamRequest(0).ByteSizeLong());
    EXPECT_EQ(observed.response_on_finish, MakeStreamResponse(kStreamMessages - 1).ByteSizeLong());
}

UTEST_F_MT(ServerMessageSizeTest, BidirectionalStreamingRunsRecvAndSendHooksInParallel, 2) {
    ON_CALL(Service(), Chat)
        .WillByDefault([](ugrpc::server::CallContext&, tests::UnitTestServiceGmock::ChatReaderWriter& stream) {
            auto write_task = engine::AsyncNoTracing([&stream] {
                for (std::size_t message = 0; message < kBidirectionalMessages; ++message) {
                    stream.Write(MakeStreamResponse(message % kStreamMessages));
                }
            });

            StreamRequest request;
            while (stream.Read(request)) {
            }
            write_task.Get();
            return tests::UnitTestServiceGmock::ChatResult{grpc::Status::OK};
        });

    std::atomic<std::size_t> received_hooks{0};
    std::atomic<std::size_t> sent_hooks{0};
    std::atomic<std::size_t> observed_sizes_sink{0};
    ON_CALL(Middleware(), PostRecvMessage)
        .WillByDefault([&](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            observed_sizes_sink += context.GetLatestRequestSizeBytes().value_or(0);
            ++received_hooks;
        });
    ON_CALL(Middleware(), PreSendMessage)
        .WillByDefault([&](ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) {
            observed_sizes_sink += context.GetLatestResponseSizeBytes().value_or(0);
            ++sent_hooks;
        });

    auto stream = Client().Chat();
    auto client_write_task = engine::AsyncNoTracing([&stream] {
        for (std::size_t message = 0; message < kBidirectionalMessages; ++message) {
            if (!stream.Write(MakeStreamRequest(message % kStreamMessages))) {
                return false;
            }
        }
        return stream.WritesDone();
    });

    std::size_t responses_count = 0;
    StreamResponse response;
    while (stream.Read(response)) {
        ++responses_count;
    }

    EXPECT_TRUE(client_write_task.Get());
    EXPECT_EQ(responses_count, kBidirectionalMessages);
    EXPECT_EQ(received_hooks.load(), kBidirectionalMessages);
    EXPECT_EQ(sent_hooks.load(), kBidirectionalMessages);
}

UTEST_F(ClientMessageSizeTest, BidirectionalStreamingHasNoWireSizes) {
    ON_CALL(Service(), Chat)
        .WillByDefault([](ugrpc::server::CallContext&, tests::UnitTestServiceGmock::ChatReaderWriter& stream) {
            StreamRequest request;
            std::size_t message = 0;
            while (stream.Read(request)) {
                stream.Write(MakeStreamResponse(message++));
            }
            return tests::UnitTestServiceGmock::ChatResult{grpc::Status::OK};
        });
    const auto& observed = ObserveSizes();

    auto stream = Client().Chat();
    StreamResponse response;
    for (std::size_t message = 0; message < kStreamMessages; ++message) {
        EXPECT_TRUE(stream.Write(MakeStreamRequest(message)));
        EXPECT_TRUE(stream.Read(response));
    }
    EXPECT_TRUE(stream.WritesDone());
    EXPECT_FALSE(stream.Read(response));

    const std::vector<std::optional<std::size_t>> no_sizes(kStreamMessages, std::nullopt);
    EXPECT_EQ(observed.request_on_send, no_sizes);
    EXPECT_EQ(observed.response_on_recv, no_sizes);
    EXPECT_EQ(observed.request_on_finish, std::nullopt);
    EXPECT_EQ(observed.response_on_finish, std::nullopt);
}

USERVER_NAMESPACE_END
