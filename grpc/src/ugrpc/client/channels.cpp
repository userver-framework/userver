#include <userver/ugrpc/client/channels.hpp>

#include <algorithm>

#include <grpcpp/create_channel.h>

#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace impl {

namespace {

constexpr engine::Deadline::Duration kMinExponentialBackoff = std::chrono::milliseconds{100};
constexpr engine::Deadline::Duration kMaxExponentialBackoff = std::chrono::seconds{30};

class Backoff {
public:
    Backoff() = default;

    engine::Deadline::Duration NextBackoff() {
        const auto current_backoff = current_backoff_;
        current_backoff_ = GetNextBackoff(current_backoff);
        return current_backoff;
    }

private:
    static engine::Deadline::Duration GetNextBackoff(engine::Deadline::Duration backoff) {
        return std::min(kMaxExponentialBackoff, backoff * 2);
    }

    engine::Deadline::Duration current_backoff_{kMinExponentialBackoff};
};

std::optional<grpc_connectivity_state> GetFinalChannelState(grpc::Channel& channel) {
    // A potentially-blocking call.
    const auto state = channel.GetState(true);
    if (state == ::GRPC_CHANNEL_READY) {
        return state;
    }
    if (state == ::GRPC_CHANNEL_SHUTDOWN) {
        return state;
    }
    return std::nullopt;
}

[[nodiscard]] bool DoTryWaitForConnected(grpc::Channel& channel, engine::Deadline deadline) {
    if (const auto state = GetFinalChannelState(channel); state.has_value()) {
        return *state == ::GRPC_CHANNEL_READY;
    }

    Backoff backoff;
    auto attempt_deadline = engine::Deadline::Clock::now();

    while (true) {
        attempt_deadline += backoff.NextBackoff();
        if (deadline < engine::Deadline::FromTimePoint(attempt_deadline)) {
            break;
        }

        engine::InterruptibleSleepUntil(attempt_deadline);

        if (const auto state = GetFinalChannelState(channel); state.has_value()) {
            return *state == ::GRPC_CHANNEL_READY;
        }
    }

    if (const auto state = GetFinalChannelState(channel); state.has_value()) {
        return *state == ::GRPC_CHANNEL_READY;
    }
    return false;
}

}  // namespace

[[nodiscard]] bool TryWaitForConnected(
    const ClientData& client_data,
    engine::Deadline deadline,
    engine::TaskProcessor& blocking_task_processor
) {
    UINVARIANT(deadline.IsReachable(), "Deadline must be reachable");
    const auto stub_state = client_data.GetStubState();
    const auto& channels = stub_state->stubs.channels;

    std::vector<engine::TaskWithResult<bool>> tasks{};
    tasks.reserve(channels.size());
    for (auto& channel : channels) {
        tasks.emplace_back(
            engine::CriticalAsyncNoTracing(blocking_task_processor, DoTryWaitForConnected, std::ref(*channel), deadline)
        );
    }
    const auto results = engine::GetAll(tasks);
    return std::ranges::all_of(results, std::identity{});
}

}  // namespace impl

std::shared_ptr<grpc::Channel> MakeChannel(
    engine::TaskProcessor& blocking_task_processor,
    std::shared_ptr<grpc::ChannelCredentials> channel_credentials,
    const std::string& endpoint
) {
    // Spawn a blocking task creating a gRPC channel
    // This is third party code, no use of span inside it
    return engine::AsyncNoTracing(
               blocking_task_processor,
               grpc::CreateChannel,
               ugrpc::impl::ToGrpcString(endpoint),
               std::ref(channel_credentials)
    )
        .Get();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
