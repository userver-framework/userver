#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include <google/protobuf/arena.h>

#include <benchmark/benchmark.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/protobuf/arena.hpp>
#include <userver/ugrpc/byte_buffer_utils.hpp>
#include <userver/ugrpc/client/generic_client.hpp>
#include <userver/ugrpc/server/impl/message_storage.hpp>
#include <userver/ugrpc/server/result.hpp>
#include <userver/ugrpc/tests/service.hpp>

#include <tests/arena_client.usrv.pb.hpp>
#include <tests/arena_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

enum class ArenaMode { kDisabled, kRequests, kRequestsAndResponses };

using sample::ugrpc::arena::BuildRequest;
using sample::ugrpc::arena::Payload;

void BuildPayload(Payload& response, const BuildRequest& request) {
    const std::string value(request.value_bytes(), 'a');
    response.set_id(1);
    response.mutable_children()->Reserve(request.rows());
    for (std::uint32_t row = 0; row < request.rows(); ++row) {
        auto& child = *response.add_children();
        child.set_id(row + 1);
        child.mutable_children()->Reserve(request.fields_per_row());
        for (std::uint32_t field = 0; field < request.fields_per_row(); ++field) {
            auto& leaf = *child.add_children();
            leaf.set_id(field + 1);
            leaf.add_values(value);
        }
    }
}

BuildRequest MakeBuildRequest(const benchmark::State& state) {
    BuildRequest request;
    request.set_rows(state.range(1));
    request.set_fields_per_row(state.range(2));
    request.set_value_bytes(state.range(3));
    return request;
}

void SetBuildCounters(benchmark::State& state, const BuildRequest& request) {
    Payload response;
    BuildPayload(response, request);
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(response.ByteSizeLong()));
    state.counters["messages"] = 1 + request.rows() * (1 + request.fields_per_row());
    constexpr const char* kModeNames[] = {"heap", "request-arena", "full-arena"};
    state.SetLabel(kModeNames[state.range(0)]);
}

void BuildArguments(benchmark::internal::Benchmark* benchmark) {
    benchmark->ArgNames({"mode", "rows", "fields", "value_bytes"});
    for (const auto mode : {0, 1, 2}) {
        benchmark->Args({mode, 0, 0, 0});
        benchmark->Args({mode, 1024, 8, 8});
        benchmark->Args({mode, 1024, 8, 128});
    }
}

class ArenaBenchmarkService final : public sample::ugrpc::arena::ArenaBenchmarkServiceBase {
public:
    explicit ArenaBenchmarkService(ArenaMode mode)
        : mode_(mode)
    {}

    EchoResult Echo(CallContext& context, sample::ugrpc::arena::Payload&& request) override {
        if (mode_ == ArenaMode::kRequestsAndResponses) {
            auto& response = *protobuf::CreateArenaMessage<Payload>(*context.GetArena());
            response.CopyFrom(request);
            return EchoResult::BorrowArenaResponse(response);
        }
        return sample::ugrpc::arena::Payload(request);
    }

    BuildResult Build(CallContext& context, BuildRequest&& request) override {
        using Storage = ugrpc::server::impl::MessageStorage<Payload>;
        auto storage =
            mode_ == ArenaMode::kRequestsAndResponses ? Storage::AllocateOnArena(*context.GetArena()) : Storage{};
        auto& response = storage.Get();
        BuildPayload(response, request);
        if (mode_ == ArenaMode::kRequestsAndResponses) {
            return BuildResult::BorrowArenaResponse(response);
        }
        return std::move(response);
    }

private:
    const ArenaMode mode_;
};

void ArenaUnary(benchmark::State& state) {
    const logging::DefaultLoggerGuard logger_guard{logging::impl::MakeNoopLoggerForTests()};
    const auto mode = static_cast<ArenaMode>(state.range(0));
    sample::ugrpc::arena::Payload request;
    for (std::int64_t i = 0; i < state.range(1); ++i) {
        auto& child = *request.add_children();
        for (int j = 0; j < 16; ++j) {
            child.add_values(std::string(128, 'a'));
        }
    }
    engine::RunStandalone(2, [&] {
        ugrpc::tests::Service<ArenaBenchmarkService>
            service({.use_protobuf_arena = mode != ArenaMode::kDisabled}, std::in_place, mode);
        auto client = service.MakeClient<sample::ugrpc::arena::ArenaBenchmarkServiceClient>();
        for (auto _ : state) {
            auto response = client.Echo(request);
            benchmark::DoNotOptimize(response);
        }
    });
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(request.ByteSizeLong()));
}

BENCHMARK(ArenaUnary)
    ->ArgsProduct({{0, 1, 2}, {0, 1, 64}})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

enum class ClientMode { kTyped, kRaw };

template <ClientMode Mode>
void ArenaUnaryBuild(benchmark::State& state) {
    const logging::DefaultLoggerGuard logger_guard{logging::impl::MakeNoopLoggerForTests()};
    const auto mode = static_cast<ArenaMode>(state.range(0));
    const auto request = MakeBuildRequest(state);
    Payload expected;
    BuildPayload(expected, request);
    engine::RunStandalone(2, [&] {
        ugrpc::tests::Service<ArenaBenchmarkService>
            service({.use_protobuf_arena = mode != ArenaMode::kDisabled}, std::in_place, mode);
        auto typed_client = service.MakeClient<sample::ugrpc::arena::ArenaBenchmarkServiceClient>();
        auto raw_client = service.MakeClient<ugrpc::client::GenericClient>();
        const auto wire_request = ugrpc::SerializeToByteBuffer(request);
        const auto call = [&] {
            if constexpr (Mode == ClientMode::kTyped) {
                return typed_client.Build(request);
            } else {
                return raw_client.UnaryCall("sample.ugrpc.arena.ArenaBenchmarkService/Build", wire_request);
            }
        };
        auto warmup = call();
        Payload actual;
        if constexpr (Mode == ClientMode::kTyped) {
            actual = std::move(warmup);
        } else if (!ugrpc::ParseFromByteBuffer(std::move(warmup), actual)) {
            state.SkipWithError("Invalid response protobuf");
            return;
        }
        if (actual.SerializeAsString() != expected.SerializeAsString()) {
            state.SkipWithError("Response contents differ from the expected payload");
            return;
        }
        for (auto _ : state) {
            auto response = call();
            benchmark::DoNotOptimize(response);
        }
    });
    SetBuildCounters(state, request);
}

BENCHMARK_TEMPLATE(ArenaUnaryBuild, ClientMode::kTyped)
    ->Name("ArenaUnaryBuildTyped")
    ->Apply(BuildArguments)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(ArenaUnaryBuild, ClientMode::kRaw)
    ->Name("ArenaUnaryBuildRaw")
    ->Apply(BuildArguments)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

void ArenaResponseConstruction(benchmark::State& state) {
    const auto request = MakeBuildRequest(state);
    for (auto _ : state) {
        std::optional<google::protobuf::Arena> arena;
        if (static_cast<ArenaMode>(state.range(0)) == ArenaMode::kRequestsAndResponses) {
            arena.emplace();
        }
        using Storage = ugrpc::server::impl::MessageStorage<Payload>;
        auto storage = arena ? Storage::AllocateOnArena(*arena) : Storage{};
        auto& response = storage.Get();
        BuildPayload(response, request);
        auto result =
            arena ? ugrpc::server::Result<Payload>::BorrowArenaResponse(response)
                  : ugrpc::server::Result<Payload>{std::move(response)};
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
    SetBuildCounters(state, request);
}

BENCHMARK(ArenaResponseConstruction)
    ->ArgsProduct({{0, 2}, {1024}, {8}, {8, 128}})
    ->ArgNames({"mode", "rows", "fields", "value_bytes"})
    ->Unit(benchmark::kMicrosecond);

}  // namespace

USERVER_NAMESPACE_END
