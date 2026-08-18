#pragma once

#include <memory>

#include <nghttp2/nghttp2.h>
#include <boost/container/small_vector.hpp>

#include <server/http/http_request_constructor.hpp>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {
class Socket;
}

namespace server::http {

class Http2StreamReadPipe;

class Stream final {
public:
    using Id = utils::StrongTypedef<struct IdTag, std::int32_t>;

    Stream(
        HttpRequestConstructor::Config config,
        const HandlerInfoIndex& handler_info_index,
        request::ResponseDataAccounter& data_accounter,
        engine::io::Sockaddr remote_address,
        Id id
    );

    Stream(const Stream&) = delete;
    Stream(Stream&&) = delete;
    Stream& operator=(const Stream&) = delete;
    Stream& operator=(Stream&&) = delete;

    Id GetId() const;
    HttpRequestConstructor& RequestConstructor();
    bool IsDeferred() const;
    void SetDeferred(bool deferred);
    void SetEnd(bool end);
    bool IsStreaming() const;
    void SetStreaming(bool streaming);

    /// @name RFC 8441 extended CONNECT
    /// The `:method` and the `:protocol` pseudo-headers may arrive in any order, so the
    /// effective method of a CONNECT stream is only known once the header block is complete.
    /// @{
    void SetConnect();
    bool IsConnect() const;
    void SetUpgradeProtocol(std::string_view protocol);
    std::string_view GetUpgradeProtocol() const;

    /// @brief Hands the stream over to a tunnelled protocol: from now on DATA frames
    /// are its incoming bytes rather than a request body.
    void SetReadPipe(std::shared_ptr<Http2StreamReadPipe> pipe);
    /// @returns nullptr unless the stream was handed over to a tunnelled protocol.
    const std::shared_ptr<Http2StreamReadPipe>& GetReadPipe() const;
    /// @}

    bool CheckUrlComplete();
    void PushChunk(std::string&& chunk);
    ssize_t GetMaxSize(std::size_t max_len, std::uint32_t* flags);
    void Send(engine::io::RwBase& socket, std::string_view data_frame_header, std::size_t max_len);
    nghttp2_data_provider* GetNativeProvider() { return &nghttp2_provider_; }

private:
    bool url_complete_{false};
    HttpRequestConstructor constructor_;
    const Id id_;
    // Extended CONNECT
    bool is_connect_{false};
    std::string upgrade_protocol_{};
    std::shared_ptr<Http2StreamReadPipe> read_pipe_{};
    // Body sending
    nghttp2_data_provider nghttp2_provider_{};
    boost::container::small_vector<std::string, 16> chunks_{};
    std::size_t pos_in_first_chunk_{0};
    // for the streaming API
    bool is_streaming_{false};
    bool is_end_{false};
    bool is_deferred_{false};
};

}  // namespace server::http

USERVER_NAMESPACE_END
