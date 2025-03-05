#include <userver/ugrpc/tests/client_middleware_pipeline.hpp>

#include <ugrpc/client/middlewares/log/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

namespace {

static const ugrpc::client::middlewares::log::Settings kLogSettings{};

}  // namespace

SimpleClientMiddlewarePipeline::SimpleClientMiddlewarePipeline()
    : impl::ClientPipeline({
          std::make_shared<ugrpc::client::middlewares::log::Middleware>(kLogSettings),
          std::make_shared<ugrpc::client::middlewares::deadline_propagation::Middleware>(),
      }) {}

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
