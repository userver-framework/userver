#include "http2_request_parser.hpp"

#include <server/http/http_request_parser.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

constexpr std::string_view kHttp2SettingsHeader{"HTTP2-Settings: "};
constexpr std::size_t kClientMagicSize{24};

constexpr std::size_t kSettingsSize = 1;
constexpr nghttp2_settings_entry kDefaultSettings[kSettingsSize] = {
    {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS,
     server::http::kDefaultMaxConcurrentStreams}};

void CheckConnectionHealthy(nghttp2_session* session) {
  if (nghttp2_session_want_read(session) &&
      nghttp2_session_want_write(session)) {
    throw std::runtime_error{"Connection is broken"};
  }
}

void ThrowWithInfoMsgIfErr(std::string_view msg, int error_code) {
  if (error_code != 0) {
    throw std::runtime_error{
        fmt::format("{}: {}", msg, nghttp2_strerror(error_code))};
  }
}

Http2RequestParser& GetParser(void* user_data) {
  UASSERT(user_data);
  return *static_cast<Http2RequestParser*>(user_data);
}

Http2RequestParser::StreamData& GetStreamData(nghttp2_session* session,
                                              int stream_id) {
  auto* stream_data = static_cast<Http2RequestParser::StreamData*>(
      nghttp2_session_get_stream_user_data(session, stream_id));
  UASSERT(stream_data);
  return *stream_data;
}

}  // namespace

Http2RequestParser::StreamData::StreamData(
    HttpRequestConstructor::Config config,
    const HandlerInfoIndex& handler_info_index,
    request::ResponseDataAccounter& data_accounter, StreamId stream_id_,
    engine::io::Sockaddr remote_address)
    : constructor(config, handler_info_index, data_accounter, remote_address),
      stream_id(stream_id_) {}

int Http2RequestParser::FinalizeRequest(StreamData& stream_data) {
  try {
    stream_data.constructor.SetResponseStreamId(stream_data.stream_id);
    stream_data.constructor.ParseUrl();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Can't parse a url: " << ex;
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  if (auto request = stream_data.constructor.Finalize()) {
    SubmitRequest(std::move(request));
  } else {
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  return 0;
}

Http2RequestParser::Http2RequestParser(
    const HandlerInfoIndex& handler_info_index,
    const request::HttpRequestConfig& request_config,
    OnNewRequestCb&& on_new_request_cb, net::ParserStats& stats,
    request::ResponseDataAccounter& data_accounter,
    engine::io::Sockaddr remote_address)
    : handler_info_index_(handler_info_index),
      request_constructor_config_(request_config),
      on_new_request_cb_(std::move(on_new_request_cb)),
      stats_(stats),
      data_accounter_(data_accounter),
      remote_address_(remote_address),
      http1_parser_(std::make_unique<HttpRequestParser>(
          handler_info_index_, request_config,
          OnNewRequestCb{on_new_request_cb_}, stats, data_accounter_,
          remote_address)) {
  nghttp2_session_callbacks* callbacks{nullptr};
  UINVARIANT(nghttp2_session_callbacks_new(&callbacks) == 0,
             "Failed to init callbacks for HTTP/2.0");

  utils::FastScopeGuard delete_guard{
      [&callbacks]() noexcept { nghttp2_session_callbacks_del(callbacks); }};

  nghttp2_session_callbacks_set_send_callback(callbacks, OnSend);
  nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, OnFrameRecv);
  nghttp2_session_callbacks_set_on_stream_close_callback(callbacks,
                                                         OnStreamClose);
  nghttp2_session_callbacks_set_on_header_callback(callbacks, OnHeader);
  nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks,
                                                          OnBeginHeaders);
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks,
                                                            OnDataChunkRecv);

  nghttp2_session* session{nullptr};
  UINVARIANT(nghttp2_session_server_new(&session, callbacks, this) == 0,
             "Failed to init session for HTTP/2.0");
  UASSERT(session);
  session_ = SessionPtr(session, nghttp2_session_del);

  int rv = nghttp2_submit_settings(session_.get(), NGHTTP2_FLAG_NONE,
                                   kDefaultSettings, kSettingsSize);
  ThrowWithInfoMsgIfErr("Error when submit settings", rv);
  rv = nghttp2_session_send(session_.get());
  ThrowWithInfoMsgIfErr("Error when session send", rv);
}

int Http2RequestParser::OnFrameRecv(nghttp2_session* session,
                                    const nghttp2_frame* frame,
                                    void* user_data) {
  auto& parser = GetParser(user_data);
  UASSERT(session);
  UASSERT(frame);

  switch (frame->hd.type) {
    case NGHTTP2_RST_STREAM: {
      parser.RemoveStreamData(frame->rst_stream.hd.stream_id);
      LOG_ERROR() << fmt::format("RESET stream request. Error code {}",
                                 frame->rst_stream.error_code);
      CheckConnectionHealthy(parser.session_.get());
    } break;
    case NGHTTP2_GOAWAY: {
      ++parser.stats_.http2_stats.goaway_streams_;
      CheckConnectionHealthy(parser.session_.get());
    } break;
    case NGHTTP2_WINDOW_UPDATE: {
      LOG_INFO() << "NGHTTP2_WINDOW_UPDATE";

    } break;
    case NGHTTP2_SETTINGS: {
      LOG_INFO() << "NGHTTP2_SETTINGS";
      for (std::size_t i = 0; i < frame->settings.niv; i++) {
        const nghttp2_settings_entry& entry = frame->settings.iv[i];
        // TODO handle other client settings
        if (entry.settings_id == NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS) {
          parser.max_concurrent_streams_ = entry.value;
        }
      }
    } break;
    case NGHTTP2_DATA:
    case NGHTTP2_HEADERS: {
      if (!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)) {
        return 0;
      }
      // End of the request
      auto& stream_data = GetStreamData(session, frame->hd.stream_id);
      return parser.FinalizeRequest(stream_data);
    } break;
  }
  return 0;
}

int Http2RequestParser::OnHeader(nghttp2_session*, const nghttp2_frame* frame,
                                 const uint8_t* name, size_t namelen,
                                 const uint8_t* value, size_t valuelen, uint8_t,
                                 void* user_data) {
  auto& parser = GetParser(user_data);
  UASSERT(frame);
  switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
      if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        break;
      }
      auto& stream_data =
          GetStreamData(parser.session_.get(), frame->hd.stream_id);
      const std::string_view hname(reinterpret_cast<const char*>(name),
                                   namelen);
      const std::string_view hvalue(reinterpret_cast<const char*>(value),
                                    valuelen);

      auto& ctor = stream_data.constructor;
      if (hname == USERVER_NAMESPACE::http::headers::k2::kMethod) {
        ctor.SetMethod(HttpMethodFromString(hvalue));
      } else if (hname == USERVER_NAMESPACE::http::headers::k2::kPath) {
        const std::string_view url{hvalue.data(), hvalue.size()};
        ctor.AppendUrl(url.data(), url.size());
      } else {
        ctor.AppendHeaderField(hname.data(), hname.size());
        ctor.AppendHeaderValue(hvalue.data(), hvalue.size());
      }
      break;
  }
  return 0;
}

int Http2RequestParser::OnStreamClose(nghttp2_session* session,
                                      int32_t stream_id, uint32_t error_code,
                                      void* user_data) {
  const auto* stream_data = static_cast<Http2RequestParser::StreamData*>(
      nghttp2_session_get_stream_user_data(session, stream_id));
  if (!stream_data) {
    return 0;
  }

  auto& parser = GetParser(user_data);
  parser.RemoveStreamData(stream_id);

  ++parser.stats_.http2_stats.streams_close_;
  LOG_DEBUG() << fmt::format("The stream {} was closed with code {}", stream_id,
                             error_code);

  return 0;
}

int Http2RequestParser::OnBeginHeaders(nghttp2_session*,
                                       const nghttp2_frame* frame,
                                       void* user_data) {
  auto& parser = GetParser(user_data);

  UASSERT(frame);
  if (frame->hd.type != NGHTTP2_HEADERS ||
      frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    return 0;
  }

  parser.RegisterStreamData(frame->hd.stream_id);
  ++parser.stats_.http2_stats.streams_count_;

  return 0;
}

int Http2RequestParser::OnDataChunkRecv(nghttp2_session* session,
                                        uint8_t /*flags*/, int32_t stream_id,
                                        const uint8_t* data, size_t len,
                                        void* /*user_data*/) {
  auto& stream_data = GetStreamData(session, stream_id);

  const std::string_view sdata(reinterpret_cast<const char*>(data), len);
  stream_data.constructor.AppendBody(sdata.data(), sdata.size());
  return 0;
}

long Http2RequestParser::OnSend(nghttp2_session*, const uint8_t*, size_t, int,
                                void*) {
  // TODO: Here we can send body without copying
  return NGHTTP2_ERR_WOULDBLOCK;
}

void Http2RequestParser::RegisterStreamData(StreamId stream_id) {
  if (streams_.size() >= max_concurrent_streams_) {
    UINVARIANT(false, "Not implemented for HTTP/2.0");
    return;
  }
  auto [iter, success] = streams_.try_emplace(
      stream_id, request_constructor_config_, handler_info_index_,
      data_accounter_, stream_id, remote_address_);
  if (!success) {
    ++stats_.http2_stats.streams_parse_error_;
    throw std::runtime_error{
        fmt::format("Double register of the stream with id = {}", stream_id)};
  }

  stats_.parsing_request_count.Add(1);
  auto& stream_data = iter->second;
  stream_data.constructor.SetHttpMajor(2);
  stream_data.constructor.SetHttpMinor(0);

  const int res = nghttp2_session_set_stream_user_data(session_.get(),
                                                       stream_id, &stream_data);
  ThrowWithInfoMsgIfErr("Cannot to set stream user data", res);
}

void Http2RequestParser::RemoveStreamData(StreamId stream_id) {
  const auto it = streams_.find(stream_id);
  UASSERT(it != streams_.end());
  streams_.erase(it);
  stats_.parsing_request_count.Subtract(1);
}

void Http2RequestParser::SubmitRequest(
    std::shared_ptr<request::RequestBase>&& request) {
  on_new_request_cb_(std::move(request));
}

bool Http2RequestParser::Parse(const char* data, size_t datalen) {
  if (!std::exchange(upgrade_completed_, true)) {
    if (DoUpgrade({data, datalen})) {
      return true;
    } else {
      LOG_INFO() << "Client already uses http2";
    }
  }
  int readlen = nghttp2_session_mem_recv(
      session_.get(), reinterpret_cast<const uint8_t*>(data), datalen);
  if (readlen < 0) {
    LOG_ERROR() << fmt::format("Fatal error in Parse: {}",
                               nghttp2_strerror(readlen));
    ++stats_.http2_stats.streams_parse_error_;
    return false;
  }
  if (static_cast<std::size_t>(readlen) != datalen) {
    LOG_ERROR() << fmt::format("Parsed = {} but expected {}", readlen, datalen);
    return false;
  }
  return true;
}

bool Http2RequestParser::DoUpgrade(std::string_view data) {
  // TODO: this is a very bad way to detect HTTP/1.1
  if (const auto pos = data.find(kHttp2SettingsHeader);
      pos != std::string_view::npos) {
    const std::size_t shift = pos + kHttp2SettingsHeader.size();
    const std::string_view client_magic(data.data() + shift, kClientMagicSize);
    UpgradeToHttp2(client_magic);
    // Parse HTTP/1.1
    http1_parser_->Parse(data.data(), data.size());
    return true;
  }
  return false;
}

void Http2RequestParser::UpgradeToHttp2(std::string_view client_magic) {
  const auto settings_payload = crypto::base64::Base64UrlDecode(client_magic);
  auto session = session_.get();
  const int rv = nghttp2_session_upgrade2(
      session, reinterpret_cast<const uint8_t*>(settings_payload.data()),
      settings_payload.size(), 0, nullptr);
  UASSERT_MSG(rv == 0, "Error during sending http2 settings");
}

}  // namespace server::http

USERVER_NAMESPACE_END
