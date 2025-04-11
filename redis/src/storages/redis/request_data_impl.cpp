#include "request_data_impl.hpp"

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

void Wait(impl::Request& request) {
    try {
        request.Get();
    } catch (const std::exception& ex) {
        LOG_WARNING() << "exception in request.Get(): " << ex;
    }
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
