#include <server/http/http2_session.hpp>

#include <server/http/http_request_parser.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

constexpr std::size_t kSettingsSize = 2;

constexpr nghttp2_settings_entry kDefaultSettings[kSettingsSize] = {
    {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, kDefaultMaxConcurrentStreams},
    {NGHTTP2_SETTINGS_MAX_FRAME_SIZE, kDefaultStringBufferSize}};

int CheckConnectionHealthy(nghttp2_session* session) {
  if (nghttp2_session_want_read(session) == 0 &&
      nghttp2_session_want_write(session) == 0) {
    // TODO: throw?
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

void ThrowIfErr(int error_code, std::string_view msg) {
  if (error_code != 0) {
    throw std::runtime_error{
        fmt::format("{}: {}", msg, nghttp2_strerror(error_code))};
  }
}

Http2Session& GetParser(void* user_data) {
  UASSERT(user_data);
  return *static_cast<Http2Session*>(user_data);
}

}  // namespace

Http2Session::StreamData::StreamData(
    HttpRequestConstructor::Config config,
    const HandlerInfoIndex& handler_info_index,
    request::ResponseDataAccounter& data_accounter, StreamId stream_id_,
    engine::io::Sockaddr remote_address)
    : constructor(config, handler_info_index, data_accounter, remote_address),
      stream_id(stream_id_) {}

bool Http2Session::StreamData::CheckUrlComplete() {
  if (url_complete) return true;
  url_complete = true;
  try {
    constructor.ParseUrl();
  } catch (const std::exception& e) {
    LOG_WARNING() << "Can't parse a url: " << e;
    return false;
  }
  return true;
}

Http2Session::Http2Session(const HandlerInfoIndex& handler_info_index,
                           const request::HttpRequestConfig& request_config,
                           OnNewRequestCb&& on_new_request_cb,
                           net::ParserStats& stats,
                           request::ResponseDataAccounter& data_accounter,
                           engine::io::Sockaddr remote_address,
                           engine::io::RwBase* socket)
    : handler_info_index_(handler_info_index),
      request_constructor_config_(request_config),
      on_new_request_cb_(std::move(on_new_request_cb)),
      data_accounter_(data_accounter),
      stats_(stats),
      remote_address_(remote_address),
      socket_(socket) {
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

  auto rv = nghttp2_submit_settings(session_.get(), NGHTTP2_FLAG_NONE,
                                    kDefaultSettings, kSettingsSize);
  ThrowIfErr(rv, "Error when submit settings");
  rv = nghttp2_session_send(session_.get());
  ThrowIfErr(rv, "Error when session send");

  streams_.reserve(kDefaultMaxConcurrentStreams);
}

int Http2Session::OnFrameRecv(nghttp2_session* session,
                              const nghttp2_frame* frame, void* user_data) {
  auto& parser = GetParser(user_data);
  UASSERT(session);
  UASSERT(frame);

  switch (frame->hd.type) {
    case NGHTTP2_RST_STREAM: {
      const auto id = frame->rst_stream.hd.stream_id;
      if (parser.RemoveStreamData(id) != 0) {
        return NGHTTP2_ERR_CALLBACK_FAILURE;
      }
      LOG_LIMITED_WARNING()
          << fmt::format("Reset the stream request with id = {}. Error code {}",
                         id, frame->rst_stream.error_code);
      return CheckConnectionHealthy(parser.session_.get());
    } break;
    case NGHTTP2_GOAWAY: {
      ++parser.stats_.http2_stats.goaway_streams_;
      parser.is_goaway = true;
      return CheckConnectionHealthy(parser.session_.get());
    } break;
    case NGHTTP2_WINDOW_UPDATE: {
      LOG_LIMITED_TRACE() << "NGHTTP2_WINDOW_UPDATE = "
                          << frame->window_update.window_size_increment;
    } break;
    case NGHTTP2_SETTINGS: {
      for (std::size_t i = 0; i < frame->settings.niv; i++) {
        const nghttp2_settings_entry& entry = frame->settings.iv[i];
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
      auto& stream_data = parser.GetStreamByIdChecked(frame->hd.stream_id);
      try {
        stream_data.constructor.AppendHeaderField("", 0);
      } catch (const std::exception& e) {
        LOG_LIMITED_WARNING() << "can't append header field: " << e;
      }
      return parser.FinalizeRequest(stream_data);
    } break;
  }
  return 0;
}

int Http2Session::OnHeader(nghttp2_session*, const nghttp2_frame* frame,
                           const uint8_t* name, size_t namelen,
                           const uint8_t* value, size_t valuelen, uint8_t,
                           void* user_data) {
  UASSERT(frame);
  switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
      if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        break;
      }
      auto& parser = GetParser(user_data);
      auto& stream_data = parser.GetStreamByIdChecked(frame->hd.stream_id);
      const std::string_view hname(reinterpret_cast<const char*>(name),
                                   namelen);
      const std::string_view hvalue(reinterpret_cast<const char*>(value),
                                    valuelen);

      auto& ctor = stream_data.constructor;
      if (hname == USERVER_NAMESPACE::http::headers::k2::kMethod) {
        ctor.SetMethod(HttpMethodFromString(hvalue));
      } else if (hname == USERVER_NAMESPACE::http::headers::k2::kPath) {
        try {
          ctor.AppendUrl(hvalue.data(), hvalue.size());
          stream_data.CheckUrlComplete();
        } catch (const std::exception& e) {
          LOG_LIMITED_WARNING() << "can't append url: " << e;
          parser.SubmitRstStream(stream_data.stream_id);
          return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
      } else {
        try {
          ctor.AppendHeaderField(hname.data(), hname.size());
          ctor.AppendHeaderValue(hvalue.data(), hvalue.size());
        } catch (const std::exception& e) {
          LOG_LIMITED_WARNING() << "can't append header field: " << e;
          parser.SubmitRstStream(stream_data.stream_id);
          return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
      }
      break;
  }
  return 0;
}

int Http2Session::OnStreamClose(nghttp2_session*, int32_t stream_id,
                                uint32_t error_code, void* user_data) {
  auto& parser = GetParser(user_data);
  parser.RemoveStreamData(stream_id);

  ++parser.stats_.http2_stats.streams_close_;
  LOG_LIMITED_TRACE() << fmt::format("The stream {} was closed with code {}",
                                     stream_id, error_code);

  return 0;
}

int Http2Session::OnBeginHeaders(nghttp2_session*, const nghttp2_frame* frame,
                                 void* user_data) {
  UASSERT(frame);
  if (frame->hd.type != NGHTTP2_HEADERS ||
      frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    return 0;
  }

  auto& parser = GetParser(user_data);
  parser.RegisterStreamData(frame->hd.stream_id);
  ++parser.stats_.http2_stats.streams_count_;

  return 0;
}

int Http2Session::OnDataChunkRecv(nghttp2_session*, uint8_t /*flags*/,
                                  int32_t stream_id, const uint8_t* data,
                                  size_t len, void* user_data) {
  auto& parser = GetParser(user_data);
  auto& stream_data = parser.GetStreamByIdChecked(stream_id);
  try {
    const std::string_view sdata(reinterpret_cast<const char*>(data), len);
    stream_data.constructor.AppendBody(sdata.data(), sdata.size());
  } catch (const std::exception& e) {
    LOG_LIMITED_WARNING() << "can't append body: " << e;
  }
  return 0;
}

long Http2Session::OnSend(nghttp2_session* session, const uint8_t* data,
                          size_t len, int, void* user_data) {
  UASSERT(session);
  UASSERT(data);
  UASSERT(user_data);
  auto& parser = GetParser(user_data);
  UASSERT(parser.buffer_.empty());
  if (parser.socket_ != nullptr) {
    return parser.socket_->WriteAll(data, len, {});
  }
  return NGHTTP2_ERR_WOULDBLOCK;
}

int Http2Session::RegisterStreamData(StreamId stream_id) {
  if (streams_.size() >= max_concurrent_streams_) {
    LOG_LIMITED_WARNING() << fmt::format(
        "Many streams: {} >= {}", streams_.size(), max_concurrent_streams_);
    SubmitRstStream(stream_id);
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }
  auto [iter, success] = streams_.try_emplace(
      stream_id, request_constructor_config_, handler_info_index_,
      data_accounter_, stream_id, remote_address_);
  if (!success) {
    ++stats_.http2_stats.streams_parse_error_;
    LOG_LIMITED_WARNING() << fmt::format(
        "Double register of the stream with id = {}", stream_id);
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  last_stream_id_ = stream_id;
  stats_.parsing_request_count.Add(1);
  auto& stream_data = iter->second;
  stream_data.constructor.SetHttpMajor(2);
  stream_data.constructor.SetHttpMinor(0);

  const int res = nghttp2_session_set_stream_user_data(session_.get(),
                                                       stream_id, &stream_data);
  ThrowIfErr(res, "Cannot to set stream user data");
  return 0;
}

int Http2Session::RemoveStreamData(StreamId stream_id) {
  const auto it = streams_.find(stream_id);
  if (it == streams_.end()) {
    SubmitGoAway(
        NGHTTP2_REFUSED_STREAM,
        fmt::format("Remove the stream with id={} that not exist", stream_id));
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }
  streams_.erase(it);

  const int res =
      nghttp2_session_set_stream_user_data(session_.get(), stream_id, nullptr);
  ThrowIfErr(res, "Cannot to set stream user data");

  stats_.parsing_request_count.Subtract(1);
  return 0;
}

int Http2Session::SubmitRstStream(StreamId stream_id) {
  ++stats_.http2_stats.reset_streams_;
  return nghttp2_submit_rst_stream(session_.get(), NGHTTP2_FLAG_NONE, stream_id,
                                   NGHTTP2_INTERNAL_ERROR);
}
void Http2Session::SubmitGoAway(std::uint32_t error_code,
                                std::string_view msg) {
  const auto res = nghttp2_submit_goaway(
      session_.get(), NGHTTP2_FLAG_NONE, last_stream_id_, error_code,
      reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
  ThrowIfErr(res, "Error when submit GOAWAY frame");
  is_goaway = true;
}

void Http2Session::SubmitRequest(
    std::shared_ptr<request::RequestBase>&& request) {
  on_new_request_cb_(std::move(request));
}

Http2Session::StreamData& Http2Session::GetStreamByIdChecked(
    StreamId stream_id) {
  auto* stream_data = static_cast<Http2Session::StreamData*>(
      nghttp2_session_get_stream_user_data(session_.get(), stream_id));
  if (stream_data == nullptr) {
    throw std::runtime_error{
        fmt::format("There isn't stream with id={}", stream_id)};
  }
  return *stream_data;
}

bool Http2Session::Parse(std::string_view req) {
  int readlen = nghttp2_session_mem_recv(
      session_.get(), reinterpret_cast<const uint8_t*>(req.data()), req.size());
  if (readlen == NGHTTP2_ERR_FATAL) {
    return false;
  }
  if (readlen < 0) {
    LOG_LIMITED_ERROR() << fmt::format("Error in Parse: {}",
                                       nghttp2_strerror(readlen));
    ++stats_.http2_stats.streams_parse_error_;
    return false;
  }
  if (static_cast<std::size_t>(readlen) != req.size()) {
    ++stats_.http2_stats.streams_parse_error_;
    LOG_ERROR() << fmt::format("Parsed = {} but expected {}", readlen,
                               req.size());
    return false;
  }
  if (socket_ != nullptr) {
    WriteResponseToSocket();
  }
  return !is_goaway;
}

void Http2Session::UpgradeToHttp2(std::string_view client_magic) {
  const auto settings_payload = crypto::base64::Base64UrlDecode(client_magic);
  const auto rv = nghttp2_session_upgrade2(
      session_.get(), reinterpret_cast<const uint8_t*>(settings_payload.data()),
      settings_payload.size(), 0, nullptr);
  ThrowIfErr(rv, "Error during upgrade");
  RegisterStreamData(kStreamIdAfterUpgradeResponse);
}

int Http2Session::FinalizeRequest(StreamData& stream_data) {
  if (!stream_data.CheckUrlComplete()) {
    RemoveStreamData(stream_data.stream_id);
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }
  stream_data.constructor.SetResponseStreamId(stream_data.stream_id);
  if (auto request = stream_data.constructor.Finalize()) {
    SubmitRequest(std::move(request));
  } else {
    RemoveStreamData(stream_data.stream_id);
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }
  return 0;
}

std::size_t Http2Session::WriteResponseToSocket() {
  using IoData = engine::io::IoData;
  UASSERT(buffer_.empty());
  std::size_t total_size = 0;
  while (nghttp2_session_want_write(session_.get())) {
    while (true) {
      const std::uint8_t* data = nullptr;
      const std::size_t len = nghttp2_session_mem_send(session_.get(), &data);
      if (len <= 0) {
        break;
      }
      const std::string_view buf{reinterpret_cast<const char*>(data), len};
      if (buffer_.size() + buf.size() > kDefaultStringBufferSize) {
        total_size += socket_->WriteAll({IoData{buffer_.data(), buffer_.size()},
                                         IoData{buf.data(), buf.size()}},
                                        {});
        buffer_.clear();
      } else {
        buffer_.append(buf);
      }
    }
  }
  if (!buffer_.empty()) {
    total_size += socket_->WriteAll(buffer_.data(), buffer_.size(), {});
    buffer_.clear();
  }
  LOG_LIMITED_TRACE() << fmt::format("Send {} bytes to the socket", total_size);
  return total_size;
}

}  // namespace server::http

USERVER_NAMESPACE_END
