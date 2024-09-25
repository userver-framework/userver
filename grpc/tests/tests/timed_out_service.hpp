#pragma once

#include <cstddef>
#include <vector>

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

class TimedOutUnitTestService final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    tests::WaitUntilRpcDeadline(call);

    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }

  void ReadMany(ReadManyCall& call,
                ::sample::ugrpc::StreamGreetingRequest&& request) override {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("One " + request.name());
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    call.Write(response);
    response.set_name("Two " + request.name());
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    call.Write(response);
    response.set_name("Three " + request.name());
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    call.Write(response);

    tests::WaitUntilRpcDeadline(call);
    call.Finish();
  }

  void WriteMany(WriteManyCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    std::size_t reads{0};

    while (call.Read(request)) {
      ASSERT_LT(reads, std::size(kRequests));
      EXPECT_EQ(request.name(), kRequests[reads]);
      ++reads;
    }

    tests::WaitUntilRpcDeadline(call);

    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }

  void Chat(ChatCall& call) override {
    std::vector<sample::ugrpc::StreamGreetingRequest> requests(3);

    sample::ugrpc::StreamGreetingResponse response;

    for (auto& it : requests) {
      if (!call.Read(it)) {
        // It is deadline from client side
        return;
      }
    }

    response.set_name("One " + requests[0].name());
    UEXPECT_NO_THROW(call.Write(response));
    response.set_name("Two " + requests[1].name());
    UEXPECT_NO_THROW(call.Write(response));

    tests::WaitUntilRpcDeadline(call);

    response.set_name("Three " + requests[0].name());
    UEXPECT_THROW(call.WriteAndFinish(response),
                  ugrpc::server::RpcInterruptedError);
  }
};

}  // namespace tests

USERVER_NAMESPACE_END
