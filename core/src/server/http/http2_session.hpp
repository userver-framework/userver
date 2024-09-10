#pragma once

#include <nghttp2/nghttp2.h>
#include <boost/pool/object_pool.hpp>

#include <server/http/http2_writer.hpp>
#include <server/http/http_request_constructor.hpp>
#include <server/net/stats.hpp>
#include <server/request/request_parser.hpp>

#include <userver/engine/io/socket.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/utils/small_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

inline constexpr std::string_view kSwitchingProtocolResponse{
    "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: "
    "h2c\r\n\r\n"};

// nghttp2_session_upgrade2 opens the stream with id=1 and we must use it.
// See docs:
// https://nghttp2.org/documentation/nghttp2_session_upgrade2.html#nghttp2-session-upgrade2
inline constexpr std::size_t kStreamIdAfterUpgradeResponse = 1;
// So ussualy the standart frame size is 16384 bytes
inline constexpr std::size_t kDefaultStringBufferSize = 1 << 14;
inline constexpr std::size_t kDefaultMaxConcurrentStreams = 100;

struct Stream final {
  using StreamId = std::int32_t;

  Stream(HttpRequestConstructor::Config config,
         const HandlerInfoIndex& handler_info_index,
         request::ResponseDataAccounter& data_accounter,
         engine::io::Sockaddr remote_address, StreamId id);

  bool CheckUrlComplete();

  bool url_complete{false};
  HttpRequestConstructor constructor;
  const StreamId id;
  std::optional<DataBufferSender> data_buffer_sender;
};

class HttpRequestParser;

class Http2Session final : public request::RequestParser {
 public:
  using OnNewRequestCb =
      std::function<void(std::shared_ptr<request::RequestBase>&&)>;

  Http2Session(const HandlerInfoIndex& handler_info_index,
               const request::HttpRequestConfig& request_config,
               OnNewRequestCb&& on_new_request_cb, net::ParserStats& stats,
               request::ResponseDataAccounter& data_accounter,
               engine::io::Sockaddr remote_address,
               engine::io::RwBase* socket = nullptr);

  Http2Session(const Http2Session&) = delete;
  Http2Session(Http2Session&&) = delete;
  Http2Session& operator=(const Http2Session&) = delete;
  Http2Session& operator=(Http2Session&&) = delete;

  bool Parse(std::string_view req) override;

  nghttp2_session* GetNghttp2SessionPtr() { return session_.get(); }

  void UpgradeToHttp2(std::string_view client_magic);

 private:
  static int OnFrameRecv(nghttp2_session* session, const nghttp2_frame* frame,
                         void* user_data);

  static int OnHeader(nghttp2_session* session, const nghttp2_frame* frame,
                      const uint8_t* name, size_t namelen, const uint8_t* value,
                      size_t valuelen, uint8_t flags, void* user_data);

  static int OnStreamClose(nghttp2_session* session, int32_t stream_id,
                           uint32_t error_code, void* user_data);

  static int OnBeginHeaders(nghttp2_session* session,
                            const nghttp2_frame* frame, void* user_data);

  static int OnDataChunkRecv(nghttp2_session* session, uint8_t flags,
                             int32_t stream_id, const uint8_t* data, size_t len,
                             void* user_data);

  static long OnSend(nghttp2_session* session, const uint8_t* data,
                     size_t length, int flags, void* user_data);

  void RegisterStream(Stream::StreamId id);
  void RemoveStream(Stream& id);
  Stream& GetStreamChecked(Stream::StreamId id);

  void SubmitRstStream(Stream::StreamId stream_id);

  void FinalizeRequest(Stream& stream);
  std::size_t WriteResponseToSocket();
  bool ConnectionIsOk();

 private:
  friend class Http2ResponseWriter;

  using SessionPtr =
      std::unique_ptr<nghttp2_session,
                      std::function<decltype(nghttp2_session_del)>>;

  SessionPtr session_{nullptr};
  boost::object_pool<Stream> streams_pool_;

  const HandlerInfoIndex& handler_info_index_;
  const HttpRequestConstructor::Config request_constructor_config_;
  OnNewRequestCb on_new_request_cb_;
  request::ResponseDataAccounter& data_accounter_;

  net::ParserStats& stats_;
  engine::io::Sockaddr remote_address_;
  engine::io::RwBase* socket_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
