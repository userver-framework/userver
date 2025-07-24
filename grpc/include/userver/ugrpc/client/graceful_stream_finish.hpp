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
    return impl::ReadRemainingAndFinish<Reader<Response>, Response>(stream);
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
    const bool writes_done_success = stream.WritesDone();
    const std::optional<std::size_t> messages_remaining =
        impl::ReadRemainingAndFinish<ReaderWriter<Request, Response>, Response>(stream);
    return writes_done_success ? messages_remaining : std::nullopt;
}

/// @brief Gracefully finish a ping-pong style interaction
///
/// 1. Announces end-of-output to the server
/// 2. Ensures there are no more messages to read
///
/// @warning The method will hang indefenitely if the stream
///          is never interrupted or closed by the server
///
/// @return true if the operation was successful; false if there are more messages
///         in the stream to read or if the operation failed (i.e. due to task
///         cancellation, the stream being already closed for writes,
///         or the server returning an error status)
template <typename Request, typename Response>
[[nodiscard]] bool PingPongFinish(ReaderWriter<Request, Response>& stream) noexcept {
    try {
        const bool writes_done_success = stream.WritesDone();

        Response response;
        if (!stream.Read(response)) {
            // As expected, there are no more messages in the stream
            return writes_done_success;
        }

        LOG_WARNING() << "PingPongFinish: there are more messages to read from the stream";
        return false;
    } catch (const std::exception& e) {
        LOG_WARNING() << "PingPongFinish: failed to close the stream - " << e;
        return false;
    }
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
