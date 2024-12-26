#pragma once

#include <cstddef>
#include <vector>

#include <userver/ugrpc/server/exceptions.hpp>

#include <userver/utest/utest.hpp>

#include <tests/deadline_helpers.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

constexpr const char* kRequests[] = {
    "request1",
    "request2",
    "request3",
};

class TimedOutUnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        tests::WaitUntilRpcDeadlineService();

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
        response.set_name("One " + request.name());
        // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
        writer.Write(response);
        response.set_name("Two " + request.name());
        // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
        writer.Write(response);
        response.set_name("Three " + request.name());
        // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
        writer.Write(response);

        tests::WaitUntilRpcDeadlineService();
        return grpc::Status::OK;
    }

    WriteManyResult WriteMany(CallContext& /*context*/, WriteManyReader& reader) override {
        sample::ugrpc::StreamGreetingRequest request;
        std::size_t reads{0};

        while (reader.Read(request)) {
            EXPECT_LT(reads, std::size(kRequests));
            if (reads < std::size(kRequests)) {
                EXPECT_EQ(request.name(), kRequests[reads]);
            }
            ++reads;
        }

        tests::WaitUntilRpcDeadlineService();

        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }

    ChatResult Chat(CallContext& /*context*/, ChatReaderWriter& stream) override {
        std::vector<sample::ugrpc::StreamGreetingRequest> requests(3);

        sample::ugrpc::StreamGreetingResponse response;

        for (auto& it : requests) {
            if (!stream.Read(it)) {
                // It is deadline from client side
                return grpc::Status{grpc::StatusCode::UNKNOWN, "Could not read expected amount of requests"};
            }
        }

        response.set_name("One " + requests[0].name());
        UEXPECT_NO_THROW(stream.Write(response));
        response.set_name("Two " + requests[1].name());
        UEXPECT_NO_THROW(stream.Write(response));

        tests::WaitUntilRpcDeadlineService();

        response.set_name("Three " + requests[0].name());
        UEXPECT_THROW(stream.Write(response), ugrpc::server::RpcInterruptedError);
        return grpc::Status::OK;
    }
};

}  // namespace tests

USERVER_NAMESPACE_END
