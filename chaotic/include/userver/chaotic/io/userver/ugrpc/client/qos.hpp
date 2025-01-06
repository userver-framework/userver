#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/ugrpc/client/qos.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

template <typename T>
Qos Convert(const T& value, chaotic::convert::To<Qos>) {
    Qos result;
    result.timeout = value.timeout_ms;
    return result;
}

template <typename T>
T Convert(const Qos& value, chaotic::convert::To<T>) {
    T result;
    result.timeout_ms = value.timeout;
    return result;
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
