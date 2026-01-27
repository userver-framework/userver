#include <userver/ugrpc/server/impl/call_state.hpp>

#include <userver/dynamic_config/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

CallState::CallState(CallParams&& params)
    : CallParams(std::move(params)),
      statistics_scope(method_statistics),
      config_snapshot(config_source.GetSnapshot())
{}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
