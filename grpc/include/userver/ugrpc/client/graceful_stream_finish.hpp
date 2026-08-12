#pragma once

/// @file
/// @brief Utilities to gracefully finish streams

#include <cstddef>
#include <exception>
#include <optional>

#include <userver/logging/log.hpp>
#include <userver/ugrpc/client/impl/graceful_stream_finish.hpp>
#include <userver/ugrpc/client/stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Read all remaining messages from the stream and call `Finish`.
///
/// @warning The method will hang indefenitely if the stream
///          is never interrupted or closed by the server
///
/// @return the number of messages received from the stream;
///         nullopt if the operation failed (i.e. due to task
///         cancellation, the stream being already closed for reads,
///         or the server returning an error status)
template <typename Response>
std::optional<std::size_t> ReadRemainingAndFinish(Reader<Response>& stream) noexcept {
    try {
        return impl::ReadRemainingAndFinish<Reader<Response>, Response>(stream);
    } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to read remaining messages and finish - " << e;
        return std::nullopt;
    }
}

/// @brief Announce end-of-input to the server, read all
///        remaining messages from the stream and call `Finish`.
///
/// @warning The method will hang indefenitely if the stream
///          is never interrupted or closed by the server
///
/// @return the number of messages received from the stream;
///         nullopt if the operation failed (i.e. due to task
///         cancellation, the stream being already closed for
///         either reads or writes, or the server returning an error status)
template <typename Request, typename Response>
std::optional<std::size_t> ReadRemainingAndFinish(ReaderWriter<Request, Response>& stream) noexcept {
    try {
        const bool writes_done_success = stream.WritesDone();
        const std::optional<std::size_t>
            messages_remaining = impl::ReadRemainingAndFinish<ReaderWriter<Request, Response>, Response>(stream);
        return writes_done_success ? messages_remaining : std::nullopt;
    } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to read remaining messages and finish - " << e;
        return std::nullopt;
    }
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
