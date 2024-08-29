#include <benchmark/benchmark.h>

#include <google/protobuf/util/json_util.h>
#include <grpcpp/support/config.h>

#include <userver/utils/assert.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

#include <tests/secret_fields.pb.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

sample::ugrpc::SendRequest MakeSendRequest() {
  sample::ugrpc::SendRequest request;
  request.mutable_creds()->set_login("some-login-value");
  request.mutable_creds()->set_password("xxx-password-value");
  request.mutable_creds()->set_secret_code("xxx-secret-code");
  request.set_dest("some-dest");
  request.mutable_msg()->set_text("msg-text-value");
  return request;
}

sample::ugrpc::SendResponse MakeSendResponse() {
  sample::ugrpc::SendResponse response;
  response.set_delivered(true);
  response.mutable_reply()->set_text("reply-text-value");
  response.set_token("xxx-token-value");
  return response;
}

}  // namespace

void ConstructBench(benchmark::State& state) {
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    {
      auto request = MakeSendRequest();
      benchmark::DoNotOptimize(request);
    }
    {
      auto response = MakeSendResponse();
      benchmark::DoNotOptimize(response);
    }
  }
}

BENCHMARK(ConstructBench);

void CloneBench(benchmark::State& state) {
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    {
      auto request = MakeSendRequest();
      std::unique_ptr<google::protobuf::Message> request_clone{request.New()};
      request_clone->CopyFrom(request);
      benchmark::DoNotOptimize(request_clone);
    }
    {
      auto response = MakeSendResponse();
      std::unique_ptr<google::protobuf::Message> response_clone{response.New()};
      response_clone->CopyFrom(response);
      benchmark::DoNotOptimize(response_clone);
    }
  }
}

BENCHMARK(CloneBench);

void TrimBench(benchmark::State& state) {
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    {
      auto request = MakeSendRequest();
      ugrpc::impl::TrimSecrets(request);
      benchmark::DoNotOptimize(request);
    }
    {
      auto response = MakeSendResponse();
      ugrpc::impl::TrimSecrets(response);
      benchmark::DoNotOptimize(response);
    }
  }
}

BENCHMARK(TrimBench);

void CloneTrimBench(benchmark::State& state) {
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    {
      auto request = MakeSendRequest();
      std::unique_ptr<google::protobuf::Message> request_clone{request.New()};
      request_clone->CopyFrom(request);
      ugrpc::impl::TrimSecrets(*request_clone);
      benchmark::DoNotOptimize(request_clone);
    }
    {
      auto response = MakeSendResponse();
      std::unique_ptr<google::protobuf::Message> response_clone{response.New()};
      response_clone->CopyFrom(response);
      ugrpc::impl::TrimSecrets(*response_clone);
      benchmark::DoNotOptimize(response_clone);
    }
  }
}

BENCHMARK(CloneTrimBench);

void Utf8DebugStringBench(benchmark::State& state) {
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    {
      auto request = MakeSendRequest();
      auto req_log = request.Utf8DebugString();
      benchmark::DoNotOptimize(req_log);
    }
    {
      auto response = MakeSendResponse();
      auto resp_log = response.Utf8DebugString();
      benchmark::DoNotOptimize(resp_log);
    }
  }
}

BENCHMARK(Utf8DebugStringBench);

void MessageToJsonStringBench(benchmark::State& state) {
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    {
      auto request = MakeSendRequest();
      google::protobuf::util::JsonPrintOptions options;
      options.add_whitespace = false;
      grpc::string request_json;
      const auto status = google::protobuf::util::MessageToJsonString(
          request, &request_json, options);
      UINVARIANT(status.ok(), "Convert to JSON failed");
      benchmark::DoNotOptimize(request_json);
    }
    {
      auto response = MakeSendResponse();
      google::protobuf::util::JsonPrintOptions options;
      options.add_whitespace = false;
      grpc::string response_json;
      const auto status = google::protobuf::util::MessageToJsonString(
          response, &response_json, options);
      UINVARIANT(status.ok(), "Convert to JSON failed");
      benchmark::DoNotOptimize(response_json);
    }
  }
}

BENCHMARK(MessageToJsonStringBench);

void TrimLogBench(benchmark::State& state) {
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    {
      auto request = MakeSendRequest();
      ugrpc::impl::TrimSecrets(request);
      auto req_log = request.Utf8DebugString();
      benchmark::DoNotOptimize(req_log);
    }
    {
      auto response = MakeSendResponse();
      ugrpc::impl::TrimSecrets(response);
      auto resp_log = response.Utf8DebugString();
      benchmark::DoNotOptimize(resp_log);
    }
  }
}

BENCHMARK(TrimLogBench);

void CloneTrimLogBench(benchmark::State& state) {
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  for (auto _ : state) {
    {
      auto request = MakeSendRequest();
      std::unique_ptr<google::protobuf::Message> request_clone{request.New()};
      request_clone->CopyFrom(request);
      ugrpc::impl::TrimSecrets(*request_clone);
      auto req_log = request_clone->Utf8DebugString();
      benchmark::DoNotOptimize(req_log);
    }
    {
      auto response = MakeSendResponse();
      std::unique_ptr<google::protobuf::Message> response_clone{response.New()};
      response_clone->CopyFrom(response);
      ugrpc::impl::TrimSecrets(*response_clone);
      auto resp_log = response_clone->Utf8DebugString();
      benchmark::DoNotOptimize(resp_log);
    }
  }
}

BENCHMARK(CloneTrimLogBench);

}  // namespace ugrpc

USERVER_NAMESPACE_END
