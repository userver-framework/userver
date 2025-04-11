#pragma once

/// @file userver/ugrpc/server/stream.hpp
/// @brief Server streaming interfaces

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Interface to read client's requests.
///
/// This class is not thread-safe
///
/// If any method throws, further methods must not be called on the same stream.
template <class Request>
class Reader {
public:
    /// @cond
    virtual ~Reader() = default;
    /// @endcond

    /// @brief Await and read the next incoming message
    /// @param request where to put the request on success
    /// @returns `true` on success, `false` on end-of-input
    /// @throws ugrpc::server::RpcError on an RPC error
    [[nodiscard]] virtual bool Read(Request& request) = 0;
};

/// @brief Interface to write server's responses.
///
/// This class is not thread-safe
///
/// If any method throws, further methods must not be called on the same stream.
template <class Response>
class Writer {
public:
    /// @cond
    virtual ~Writer() = default;
    /// @endcond

    /// @brief Write the next outgoing message
    /// @param response the next message to write
    /// @throws ugrpc::server::RpcError on an RPC error
    virtual void Write(Response& response) = 0;

    /// @brief Write the next outgoing message
    /// @param response the next message to write
    /// @throws ugrpc::server::RpcError on an RPC error
    virtual void Write(Response&& response) = 0;
};

/// @brief Interface to both read and write messages.
///
/// This class is not thread-safe
///
/// If any method throws, further methods must not be called on the same stream.
///
/// This class allows the following concurrent calls:
///
///   - `Read`;
///   - `Write`
///
/// and there can only be one Read and one Write in flight at a time.
///
/// If any method throws, further methods must not be called on the same stream.
template <class Request, class Response>
class ReaderWriter : public Reader<Request>, public Writer<Response> {};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
