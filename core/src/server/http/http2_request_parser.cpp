// #include <functional>

#include <userver/crypto/base64.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/utils/scope_guard.hpp>

#include "http2_callbacks.hpp"
#include "http2_request_parser.hpp"
#include "http2_session_data.hpp"

#include <iostream>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

using StreamDataPtr = Http2StreamData*;

int FinalizeRequest(Http2SessionData* session_data, StreamDataPtr stream_data) {
  try {
    stream_data->constructor.SetResponseStreamId(stream_data->stream_id);
    stream_data->constructor.ParseUrl();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "can't parse url : " << ex;
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  if (auto request = stream_data->constructor.Finalize()) {
    session_data->SubmitRequest(std::move(request));
  } else {
    LOG_ERROR() << "request is null after Finalize()";
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  return 0;
}

}  // namespace

namespace nghttp2_callbacks::recv {

int OnFrameRecv(nghttp2_session* session, const nghttp2_frame* frame,
                void* user_data) {
  std::cout << "on_frame_recv callback for stream " << frame->hd.stream_id
            << std::endl;
  auto* session_data = reinterpret_cast<Http2SessionData*>(user_data);

  UASSERT(session_data);
  // UASSERT(frame->hd.type == NGHTTP2_DATA || frame->hd.type ==
  // NGHTTP2_HEADERS);
  std::cout << "\tframe type " << static_cast<size_t>(frame->hd.type)
            << std::endl;

  if (frame->hd.type == NGHTTP2_RST_STREAM) {
    std::cout << "\tclose stream reqeust. Error code "
              << frame->rst_stream.error_code << std::endl;
  }

  if (frame->hd.type == NGHTTP2_SETTINGS) {
    // size_t count = frame->settings.niv;
    // std::cout << "settings num " << count << std::endl;
    // for (size_t i = 0; i < count; ++i) {
    // std::cout << "settings id " << frame->settings.iv[i].settings_id
    //<< " value " << frame->settings.iv[i].value << std::endl;
    //}
  }

  // TODO GOAWAY -  stop recieving and finish existingd

  if (frame->hd.type != NGHTTP2_DATA && frame->hd.type != NGHTTP2_HEADERS) {
    return 0;
  }

  if (frame->hd.type == NGHTTP2_DATA) {
    std::cout << "\tdata frame padlen is " << frame->data.padlen << std::endl;
  }

  if (!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)) {
    return 0;
  }

  // End of the request
  auto* stream_data = reinterpret_cast<StreamDataPtr>(
      nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));

  // Callback can be called after on_stream_close_callback. Check stream still
  // alive.
  UASSERT(stream_data);

  return FinalizeRequest(session_data, stream_data);
}

int OnHeader(nghttp2_session* session, const nghttp2_frame* frame,
             const uint8_t* name, size_t namelen, const uint8_t* value,
             size_t valuelen, uint8_t /*flags*/, void* /*user_data*/) {
  std::cout << "on_header callback" << std::endl;
  // utils::ScopeGuard stats_guard([] { std::cout << "on_header callback exit";
  // });

  if (frame->hd.type != NGHTTP2_HEADERS) {
    return 0;
  }

  // don't really know why...
  if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    return 0;
  }

  std::cout << "\ttry get stream_data for " << frame->hd.stream_id << std::endl;
  auto* stream_data = reinterpret_cast<StreamDataPtr>(
      nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));
  std::cout << "\tgot stream_data" << std::endl;

  if (!stream_data) {
    return 0;
  }

  std::string_view hname(reinterpret_cast<const char*>(name), namelen);
  std::string_view hvalue(reinterpret_cast<const char*>(value), valuelen);
  std::cout << "\theader parsed " << hname << " : " << hvalue << std::endl;

  HttpRequestConstructor& ctor = stream_data->constructor;
  if (hname == http2::pseudo_headers::kMethod) {
    ctor.SetMethod(HttpMethodFromString(hvalue));
  } else if (hname == http2::pseudo_headers::kPath) {
    ctor.AppendUrl(hvalue.data(), hvalue.size());
  } else if (hname == http2::pseudo_headers::kScheme) {
    // TODO add scheme field to constructor
  } else if (hname == http2::pseudo_headers::kAuthority) {
    // TODO add authority field to constructor
  } else {
    ctor.AppendHeaderField(hname.data(), hname.size());
    ctor.AppendHeaderValue(hvalue.data(), hvalue.size());
  }

  return 0;
}

int OnStreamClose(nghttp2_session* session, int32_t stream_id,
                  uint32_t error_code, void* user_data) {
  std::cout << "on_stream_close " << stream_id << " callback" << std::endl;
  auto* session_data = reinterpret_cast<Http2SessionData*>(user_data);
  auto* stream_data = reinterpret_cast<StreamDataPtr>(
      nghttp2_session_get_stream_user_data(session, stream_id));

  if (!stream_data) {
    return 0;
  }

  UASSERT(session_data);
  UASSERT(stream_data);

  session_data->RemoveStreamData(stream_id);

  LOG_DEBUG() << "Stream " << stream_id << " was closed with code "
              << error_code;

  return 0;
}

int OnBeginHeaders(nghttp2_session* /*session*/, const nghttp2_frame* frame,
                   void* user_data) {
  std::cout << "on_begin_headers callback" << std::endl;
  auto* session_data = reinterpret_cast<Http2SessionData*>(user_data);

  if (frame->hd.type != NGHTTP2_HEADERS ||
      frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
    return 0;
  }

  session_data->RegisterStreamData(frame->hd.stream_id);

  return 0;
}

int OnDataChunkRecv(nghttp2_session* session, uint8_t /*flags*/,
                    int32_t stream_id, const uint8_t* data, size_t len,
                    void* /*user_data*/) {
  std::cout << "on_data_chunk callback" << std::endl;
  // UASSERT(false);
  auto* stream_data = reinterpret_cast<StreamDataPtr>(
      nghttp2_session_get_stream_user_data(session, stream_id));

  std::string_view sdata(reinterpret_cast<const char*>(data), len);
  stream_data->constructor.AppendBody(sdata.data(), sdata.size());

  return 0;
}

int ErrorCallback(nghttp2_session* /*session*/, int /*lib_error_code*/,
                  const char* /*msg*/, size_t /*len*/, void* /*user_data*/) {
  std::cout << "error_callback" << std::endl;
  UASSERT(false);
  return 0;
}

int OnExtensionChunk(nghttp2_session* /*session*/,
                     const nghttp2_frame_hd* /*hd*/, const uint8_t* /*data*/,
                     size_t /*len*/, void* /*user_data*/) {
  std::cout << "on_extension_chunk" << std::endl;
  UASSERT(false);
  return 0;
}

int OnInvalidFrameRecv(nghttp2_session* /*session*/,
                       const nghttp2_frame* /*frame*/, int /*lib_error_code*/,
                       void* /*user_data*/) {
  std::cout << "on_extension_chunk" << std::endl;
  UASSERT(false);
  return 0;
}

int OnInvalidHeader(nghttp2_session*, const nghttp2_frame*, const uint8_t* name,
                    size_t namelen, const uint8_t* value, size_t valuelen,
                    uint8_t, void*) {
  std::cout << "on_invalid_header callback" << std::endl;
  UASSERT(false);
  LOG_DEBUG() << "ERROR invalid header "
              << std::string_view{reinterpret_cast<const char*>(name), namelen}
              << std::string_view{reinterpret_cast<const char*>(value),
                                  valuelen};
  return 0;
}

}  // namespace nghttp2_callbacks::recv

Http2RequestParser::Http2RequestParser(
    const HandlerInfoIndex& handler_info_index,
    const request::HttpRequestConfig& request_config, net::ParserStats& stats,
    request::ResponseDataAccounter& data_accounter,
    OnNewRequestCb&& on_new_reqeust_cb)
    : stats_(stats),
      session_data_(std::make_unique<Http2SessionData>(
          request_config, handler_info_index, data_accounter,
          std::move(on_new_reqeust_cb))) {
  nghttp2_session_callbacks* callbacks{nullptr};
  nghttp2_session_callbacks_new(&callbacks);

  nghttp2_session_callbacks_set_on_frame_recv_callback(
      callbacks, nghttp2_callbacks::recv::OnFrameRecv);
  nghttp2_session_callbacks_set_on_header_callback(
      callbacks, nghttp2_callbacks::recv::OnHeader);
  nghttp2_session_callbacks_set_on_stream_close_callback(
      callbacks, nghttp2_callbacks::recv::OnStreamClose);
  nghttp2_session_callbacks_set_on_begin_headers_callback(
      callbacks, nghttp2_callbacks::recv::OnBeginHeaders);
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
      callbacks, nghttp2_callbacks::recv::OnDataChunkRecv);
  nghttp2_session_callbacks_set_error_callback2(
      callbacks, nghttp2_callbacks::recv::ErrorCallback);
  nghttp2_session_callbacks_set_on_extension_chunk_recv_callback(
      callbacks, nghttp2_callbacks::recv::OnExtensionChunk);
  nghttp2_session_callbacks_set_on_invalid_frame_recv_callback(
      callbacks, nghttp2_callbacks::recv::OnInvalidFrameRecv);
  nghttp2_session_callbacks_set_on_invalid_header_callback(
      callbacks, nghttp2_callbacks::recv::OnInvalidHeader);

  nghttp2_session_callbacks_set_send_data_callback(
      callbacks, nghttp2_callbacks::send::OnSendDataCallback);

  nghttp2_session* session{nullptr};
  nghttp2_session_server_new(&session, callbacks, session_data_.get());

  session_data_->SetSessionPtr(session);

  nghttp2_session_callbacks_del(callbacks);
}

bool Http2RequestParser::Parse(const char* data, size_t size) {
  ++stats_.parsing_request_count;

  // nghttp2_session_mem_recv - is sync operation and it returns only when all
  // bytes processed. So we can decrease stats at the on of scope
  utils::ScopeGuard stats_guard([this] { --stats_.parsing_request_count; });

  ssize_t ret = 0;
  {
    auto session_ptr = session_data_->GetSession().Lock();
    ret = nghttp2_session_mem_recv(
        session_ptr->get(), reinterpret_cast<const uint8_t*>(data), size);
  }

  auto signed_size = static_cast<ssize_t>(size);
  if (ret == signed_size) {
    LOG_TRACE() << "Request successfully parsed";
    return true;
  }

  switch (ret) {
    case NGHTTP2_ERR_NOMEM:
      std::cout << "err1" << std::endl;
      LOG_ERROR()
          << "Out of memory error was detected durig request processing";
      break;
    case NGHTTP2_ERR_CALLBACK_FAILURE:
      std::cout << "err2" << std::endl;
      LOG_ERROR()
          << "Error in callback during request processing. See logs for reason";
      break;
    case NGHTTP2_ERR_BAD_CLIENT_MAGIC:
      std::cout << "err3" << std::endl;
      LOG_ERROR() << "Bad client magic during reqeust processing";
      break;
    case NGHTTP2_ERR_FLOODED:
      std::cout << "err4" << std::endl;
      LOG_ERROR() << "Flooding detected. Need to close session";
      break;
    default:
      break;
  }

  if (ret >= 0 && ret < signed_size) {
    std::cout << "err5" << std::endl;
    LOG_ERROR() << "parsed=" << ret << " size=" << signed_size;
  }

  session_data_->SubmitEmptyRequest();
  return false;
}

bool Http2RequestParser::ApplySettings(
    const std::string_view settings,
    const std::vector<nghttp2_settings_entry>& server_settings) {
  auto session_ptr = session_data_->GetSession().Lock();

  const auto settings_payload = crypto::base64::Base64UrlDecode(settings);
  std::cout << "setting payload " << settings_payload.size() << std::endl;
  int rv = nghttp2_session_upgrade2(
      session_ptr->get(),
      reinterpret_cast<const uint8_t*>(settings_payload.data()),
      settings_payload.size(), 0, nullptr);

  UASSERT_MSG(rv == 0, "Error during sending http2 settings");
  // dont really know should i submit this settings
  rv = nghttp2_submit_settings(session_ptr->get(), NGHTTP2_FLAG_NONE,
                               server_settings.data(), server_settings.size());
  UASSERT_MSG(rv == 0, "Error during sending http2 settings");

  return true;
}

}  // namespace server::http

USERVER_NAMESPACE_END
