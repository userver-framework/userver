#include <userver/ugrpc/client/middlewares/testsuite/middleware.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::testsuite {

void Middleware::PostFinish(MiddlewareCallContext& context, const grpc::Status&) const {
    const auto& metadata = context.GetContext().GetServerTrailingMetadata();

    if (auto error_code = utils::FindOptional(metadata, ugrpc::impl::kXTestsuiteErrorCode)) {
        throw RpcInterruptedError(
            context.GetCallName(), fmt::format("Testsuite {}", ugrpc::impl::ToString(error_code.value()))
        );
    }
}

}  // namespace ugrpc::client::middlewares::testsuite

USERVER_NAMESPACE_END
