#include <userver/ugrpc/client/impl/method_stubs.hpp>

#include <userver/utils/rand.hpp>

#include <userver/ugrpc/client/impl/stub_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

std::size_t Next(const utils::FixedArray<std::shared_ptr<grpc::Channel>>& channels, std::size_t current) {
    if (2 == channels.size()) {
        // flip between 0 and 1
        return current ^ std::size_t{1};
    }

    // avoid repetitions
    const std::size_t next = utils::RandRange(std::size_t{1}, channels.size());
    return next != current ? next : 0;
}

std::size_t Select(const utils::FixedArray<std::shared_ptr<grpc::Channel>>& channels, std::optional<std::size_t> last) {
    if (1 == channels.size()) {
        return 0;
    }

    // intentionally do not use `value_or` to avoid `RandRange` when it is not needed
    const std::size_t current = last.has_value() ? *last : utils::RandRange(channels.size());
    if (GRPC_CHANNEL_READY == channels[current]->GetState(/*try_to_connect*/ false)) {
        return current;
    }

    return Next(channels, current);
}

}  // namespace

MethodStubs::MethodStubs(rcu::ReadablePtr<StubState>&& stub_state, const StubArray& stubs)
    : stub_state_{std::move(stub_state)},
      stubs_{stubs}
{}

ugrpc::impl::StubAny& MethodStubs::GetStub() const {
    last_ = Select(stubs_.channels, last_);
    return stubs_.stubs[*last_];
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
