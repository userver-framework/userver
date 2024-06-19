#include "http2_request_parser.hpp"

#include <userver/crypto/base64.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/http_version.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

int Http2RequestParser::FinalizeRequest(StreamData* stream_data) {
  try {
    stream_data->constructor.SetResponseStreamId(stream_data->stream_id);
    stream_data->constructor.ParseUrl();
  } catch (const std::exception& ex) {
    LOG_WARNING() << fmt::format("can't parse url: {}", ex.what());
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  LOG_ERROR() << fmt::format("[FINAL ID]: {}", stream_data->stream_id);
  if (auto request = stream_data->constructor.Finalize()) {
    SubmitRequest(std::move(request));
  } else {
    LOG_ERROR() << "request is null after Finalize()";
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  return 0;
}

Http2RequestParser::Http2RequestParser(
    const HandlerInfoIndex& handler_info_index,
    const request::HttpRequestConfig& request_config,
    OnNewRequestCb&& on_new_request_cb, net::ParserStats& stats,
    request::ResponseDataAccounter& data_accounter)
    : handler_info_index_(handler_info_index),
      request_constructor_config_{request_config},
      on_new_request_cb_(std::move(on_new_request_cb)),
      stats_(stats),
      data_accounter_(data_accounter) {
  nghttp2_session_callbacks* callbacks{nullptr};
  nghttp2_session_callbacks_new(&callbacks);

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
  nghttp2_session_server_new(&session, callbacks, this);
  UASSERT(session);
  session_ = SessionPtr(session, nghttp2_session_del);

  static const std::size_t kSettingsSize = 1;
  nghttp2_settings_entry iv[kSettingsSize] = {
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

  int rv = nghttp2_submit_settings(session_.get(), NGHTTP2_FLAG_NONE, iv,
                                   kSettingsSize);
  if (rv != 0 || nghttp2_session_send(session_.get()) != 0) {
    throw std::runtime_error{fmt::format(
        "Can't submit nghttp2 settings. Error: {}", nghttp2_strerror(rv))};
  }
}

std::string Http2RequestParser::WantWrite() {
  std::string response_buffer{};
  auto session = session_.get();
  while (nghttp2_session_want_write(session)) {
    const uint8_t* data_ptr{nullptr};
    std::size_t len = nghttp2_session_mem_send(session, &data_ptr);
    std::string_view append{reinterpret_cast<const char*>(data_ptr), len};
    LOG_ERROR() << "___append(release)___=" << append;
    response_buffer.append(append);
  }
  LOG_ERROR() << "nghttp2_session_want_write=" << response_buffer;
  return response_buffer;
}

int Http2RequestParser::OnFrameRecv(nghttp2_session* session,
                                    const nghttp2_frame* frame,
                                    void* user_data) {
  LOG_ERROR() << fmt::format("OnFrameRecv callback for stream {}",
                             frame->hd.stream_id);
  auto* parser = static_cast<Http2RequestParser*>(user_data);

  UASSERT(parser);
  UASSERT(session);
  LOG_ERROR() << fmt::format("[frame type]: {}",
                             static_cast<size_t>(frame->hd.type));

  if (frame->hd.type == NGHTTP2_RST_STREAM) {
    LOG_ERROR() << fmt::format("RESET stream reqeust. Error code {}",
                               frame->rst_stream.error_code);
    parser->stats_.http2_stats.reset_streams_++;
  }

  if (frame->hd.type == NGHTTP2_GOAWAY) {
    // TODO handle goaway
    LOG_ERROR() << "NGHTTP2_GOAWAY";
  }

  if (frame->hd.type == NGHTTP2_WINDOW_UPDATE) {
    LOG_ERROR() << "NGHTTP2_WINDOW_UPDATE";
  }
  if (frame->hd.type == NGHTTP2_SETTINGS) {
    LOG_ERROR() << "NGHTTP2_SETTINGS";
    // TODO handle client settings
  }

  if (frame->hd.type != NGHTTP2_DATA && frame->hd.type != NGHTTP2_HEADERS) {
    return 0;
  }

  if (frame->hd.type == NGHTTP2_DATA) {
    LOG_ERROR() << fmt::format("\tdata frame padlen is {}", frame->data.padlen);
  }

  if (!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)) {
    return 0;
  }

  // End of the request
  auto* stream_data = static_cast<StreamData*>(
      nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));

  // Callback can be called after on_stream_close_callback. Check stream still
  // alive.
  UASSERT(stream_data);

  return parser->FinalizeRequest(stream_data);
}

int Http2RequestParser::OnHeader(nghttp2_session* /*session*/,
                                 const nghttp2_frame* frame,
                                 const uint8_t* name, size_t namelen,
                                 const uint8_t* value, size_t valuelen,
                                 uint8_t /*flags*/, void* user_data) {
  LOG_ERROR() << fmt::format("OnHeader callback");
  auto* parser = static_cast<Http2RequestParser*>(user_data);
  UASSERT(parser);
  switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
      if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        LOG_ERROR() << "frame->headers.cat != NGHTTP2_HCAT_REQUEST";
        break;
      }
      auto* stream_data =
          static_cast<StreamData*>(nghttp2_session_get_stream_user_data(
              parser->session_.get(), frame->hd.stream_id));
      if (!stream_data) {
        LOG_ERROR() << "e_____";
        break;
      }
      std::string_view hname(reinterpret_cast<const char*>(name), namelen);
      std::string_view hvalue(reinterpret_cast<const char*>(value), valuelen);

      auto& ctor = stream_data->constructor;
      if (hname == kMethod) {
        ctor.SetMethod(HttpMethodFromString(hvalue));
        LOG_ERROR() << "h_____= " << hvalue;
      } else if (hname == kPath) {
        std::string_view url{hvalue.data(), hvalue.size()};
        LOG_ERROR() << fmt::format("url____ {}", url);
        ctor.AppendUrl(url.data(), url.size());
        LOG_ERROR() << "u_____= " << hvalue;
      } else if (hname == kScheme) {
        (void)kScheme;
        // TODO add scheme field to constructor
      } else if (hname == kAuthority) {
        // TODO add authority field to constructor
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
  LOG_ERROR() << fmt::format("OnStreamClose {} callback", stream_id);
  auto* parser = static_cast<Http2RequestParser*>(user_data);
  auto* stream_data = static_cast<StreamData*>(
      nghttp2_session_get_stream_user_data(session, stream_id));

  if (!stream_data) {
    return 0;
  }

  UASSERT(parser);
  UASSERT(stream_data);

  parser->RemoveStreamData(stream_id);

  parser->stats_.http2_stats.streams_close_++;
  LOG_DEBUG() << fmt::format("Stream {} was closed with code {}", stream_id,
                             error_code);

  return 0;
}

int Http2RequestParser::OnBeginHeaders(nghttp2_session*,
                                       const nghttp2_frame* frame,
                                       void* user_data) {
  LOG_ERROR() << "OnBeginHeaders callback";
  auto* parser = static_cast<Http2RequestParser*>(user_data);
  UASSERT(parser);

  if (frame->hd.type != NGHTTP2_HEADERS ||
      frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    LOG_ERROR() << "exit from OnBeginHeaders";
    return 0;
  }

  parser->RegisterStreamData(frame->hd.stream_id);
  parser->stats_.http2_stats.streams_count_++;

  return 0;
}

int Http2RequestParser::OnDataChunkRecv(nghttp2_session* session,
                                        uint8_t /*flags*/, int32_t stream_id,
                                        const uint8_t* data, size_t len,
                                        void* /*user_data*/) {
  LOG_ERROR() << "OnDataChunkRecv callback";
  auto* stream_data = static_cast<StreamData*>(
      nghttp2_session_get_stream_user_data(session, stream_id));

  UASSERT(stream_data);
  LOG_ERROR() << fmt::format("blen = {}", len);
  std::string_view sdata(reinterpret_cast<const char*>(data), len);
  stream_data->constructor.AppendBody(sdata.data(), sdata.size());
  return 0;
}

long Http2RequestParser::OnSend(nghttp2_session*, const uint8_t*, size_t, int,
                                void*) {
  LOG_ERROR() << "OnSend callback";
  return NGHTTP2_ERR_WOULDBLOCK;
}

void Http2RequestParser::RegisterStreamData(std::uint32_t stream_id) {
  auto [iter, success] =
      streams_.try_emplace(stream_id, request_constructor_config_,
                           handler_info_index_, data_accounter_, stream_id);
  if (!success) {
    stats_.http2_stats.streams_parse_error_++;  // TODO: Use other metric?
    throw std::runtime_error{
        fmt::format("Double register of the stream with id = {}", stream_id)};
  }

  LOG_ERROR() << "___EMPLACE_STREAM_WITH_ID=___=" << std::to_string(stream_id);

  stats_.parsing_request_count.Add(1);
  auto& stream_data = iter->second;
  LOG_ERROR() << "sets versions";
  stream_data.constructor.SetHttpMajor(2);
  stream_data.constructor.SetHttpMinor(0);

  nghttp2_session_set_stream_user_data(session_.get(), stream_id, &stream_data);
}

void Http2RequestParser::RemoveStreamData(std::uint32_t stream_id) {
  streams_.erase(stream_id);
  stats_.parsing_request_count.Subtract(1);
}

void Http2RequestParser::SubmitRequest(
    std::shared_ptr<request::RequestBase>&& request) {
  on_new_request_cb_(std::move(request));
}

bool Http2RequestParser::Parse(const char* data, size_t datalen) {
  LOG_ERROR() << "___ReleaseRequest___" << std::string_view{data, datalen};
  if (!upgrade_completed_) {
    upgrade_completed_ = true;
    if (DoUpgrade({data, datalen})) {
      return true;
    } else {
      LOG_ERROR() << "Client already uses the http2";
    }
  }
  int readlen = nghttp2_session_mem_recv(
      session_.get(), reinterpret_cast<const uint8_t*>(data), datalen);
  if (readlen < 0) {
    LOG_ERROR() << fmt::format("Fatal error in Parse: {}",
                               nghttp2_strerror(readlen));
    stats_.http2_stats.streams_parse_error_++;
    return false;
  }
  if (static_cast<size_t>(readlen) != datalen) {
    LOG_ERROR() << fmt::format("Parsed = {} but expected {}", readlen, datalen);
    return false;
  }
  LOG_TRACE() << "Request was successfully parsed";
  (void)url_complete_;
  return true;
}

bool Http2RequestParser::DoUpgrade(std::string_view data) {
  if (auto pos = data.find(kHttp2SettingsHeader);
      pos != std::string_view::npos) {
    const size_t shift = pos + kHttp2SettingsHeader.size();
    const std::string_view client_magic(data.data() + shift, kClientMagicSize);
    UpgradeToHttp2(client_magic,
                   {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}});

    StreamData s{request_constructor_config_, handler_info_index_,
                 data_accounter_, 1};
    s.constructor.SetUpgradeHttpResponse(true);
    s.constructor.SetResponseStreamId(1);
    SubmitRequest(s.constructor.Finalize());
    return true;
  }
  return false;
}

void Http2RequestParser::UpgradeToHttp2(
    const std::string_view settings_header,
    const std::vector<nghttp2_settings_entry>& settings) {
  const auto settings_payload =
      crypto::base64::Base64UrlDecode(settings_header);
  LOG_ERROR() << fmt::format("[SETTINGS_PAYLOAD_SIZE]= {}",
                             settings_payload.size());
  auto session = session_.get();
  int rv = nghttp2_session_upgrade2(
      session, reinterpret_cast<const uint8_t*>(settings_payload.data()),
      settings_payload.size(), 0, nullptr);

  UASSERT_MSG(rv == 0, "Error during sending http2 settings");
  rv = nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, settings.data(),
                               settings.size());
  UASSERT_MSG(rv == 0, "Error during sending http2 settings");
}

}  // namespace server::http

USERVER_NAMESPACE_END
