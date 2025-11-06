#include <userver/ugrpc/server/middlewares/access_log/log_extra.hpp>

#include <userver/ugrpc/server/middlewares/access_log/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::access_log {

void SetAdditionalLogKeys(MiddlewareCallContext& context, logging::LogExtra&& log_extra) {
    auto* extra = context.GetStorageContext().GetOptional(kLogExtraTag);

    if (extra) {
        extra->Extend(std::move(log_extra));
    } else {
        context.GetStorageContext().Emplace(kLogExtraTag, std::move(log_extra));
    }
}

}  // namespace ugrpc::server::middlewares::access_log

USERVER_NAMESPACE_END
