#pragma once

#include <cstddef>
#include <exception>
#include <optional>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <typename Stream, typename Response>
std::optional<std::size_t> ReadRemainingAndFinish(Stream& stream) {
    Response response;
    std::size_t num_messages = 0;
    while (stream.Read(response)) {
        ++num_messages;
    }
    return num_messages;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
