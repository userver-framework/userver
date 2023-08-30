#include <utility>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/ugrpc/tests/service.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/fixed_array.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

#include <benchmark/benchmark.h>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }

  void ReadMany(ReadManyCall& call,
                sample::ugrpc::StreamGreetingRequest&& request) override {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("Hello again " + request.name());
    for (int i = 0; i < request.number(); ++i) {
      response.set_number(i);
      call.Write(response);
    }
    call.Finish();
  }

  void WriteMany(WriteManyCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    int count = 0;
    while (call.Read(request)) {
      ++count;
    }
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("Hello");
    response.set_number(count);
    call.Finish(response);
  }

  void Chat(ChatCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    sample::ugrpc::StreamGreetingResponse response;
    int count = 0;
    while (call.Read(request)) {
      ++count;
      response.set_number(count);
      response.set_name("Hello " + request.name());
      call.Write(response);
    }
    call.Finish();
  }
};

using GrpcClientTest = tests::Service<UnitTestService>;

std::unique_ptr<grpc::ClientContext> PrepareClientContext() {
  auto context = std::make_unique<grpc::ClientContext>();
  context->AddMetadata("req_header", "value");
  return context;
}

void UnaryRPCPayload() {
  GrpcClientTest client_factory;
  auto client =
      client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");
  auto call_for_move = client.SayHello(out, PrepareClientContext());
  auto call = std::move(call_for_move);  // test move operation

  sample::ugrpc::GreetingResponse in;
  in = call.Finish();
  UINVARIANT("Hello " + out.name() == in.name(), "Behavior broken");
}

void UnaryRPCPayloadRepeated() {
  static constexpr std::size_t kRepetitions = 32;
  for (std::size_t i = 0; i < kRepetitions; ++i) {
    UnaryRPCPayload();
  }
}

}  // namespace

void UnaryRPC(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    for (auto _ : state) {
      UnaryRPCPayload();
    }
  });
}

BENCHMARK(UnaryRPC)->DenseRange(1, 4)->Unit(benchmark::kMicrosecond);

void BatchOfUnaryRPC(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    static constexpr std::size_t kBatchSize = 16;

    for (auto _ : state) {
      auto tasks = utils::GenerateFixedArray(kBatchSize, [](auto i) {
        return engine::AsyncNoSpan(UnaryRPCPayloadRepeated);
      });
      engine::GetAll(tasks);
    }
  });
}

BENCHMARK(BatchOfUnaryRPC)->DenseRange(1, 8)->Unit(benchmark::kMillisecond);

}  // namespace ugrpc

USERVER_NAMESPACE_END
