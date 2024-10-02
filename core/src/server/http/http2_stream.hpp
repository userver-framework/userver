#pragma once

#include <nghttp2/nghttp2.h>
#include <boost/container/small_vector.hpp>

#include <server/http/http_request_constructor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {
class Socket;
}

namespace server::http {

class Stream final {
 public:
  using StreamId = std::int32_t;

  Stream(HttpRequestConstructor::Config config,
         const HandlerInfoIndex& handler_info_index,
         request::ResponseDataAccounter& data_accounter,
         engine::io::Sockaddr remote_address, StreamId id);

  Stream(const Stream&) = delete;
  Stream(Stream&&) = delete;
  Stream& operator=(const Stream&) = delete;
  Stream& operator=(Stream&&) = delete;

  StreamId Id() const;
  HttpRequestConstructor& RequestConstructor();
  bool IsDeferred() const;
  void SetDeferred(bool deferred);
  void SetEnd(bool end);
  bool IsStreaming() const;
  void SetStreaming(bool streaming);

  bool CheckUrlComplete();
  void PushChunk(std::string&& chunk);
  ssize_t GetMaxSize(std::size_t max_len, std::uint32_t* flags);
  void Send(engine::io::Socket& socket, std::string_view data_frame_header,
            std::size_t max_len);
  nghttp2_data_provider* GetNativeProvider() { return &nghttp2_provider_; }

 private:
  bool url_complete_{false};
  HttpRequestConstructor constructor_;
  const StreamId id_;
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
