#pragma once

#include <nghttp2/nghttp2.h>

USERVER_NAMESPACE_BEGIN

namespace server::http::nghttp2_callbacks {

// implemented in http2_reqeust_parser.cpp
// TODO move implementation to http2_recv_callbacks.cpp
namespace recv {

int OnFrameRecv(nghttp2_session* session, const nghttp2_frame* frame,
                void* user_data);

int OnHeader(nghttp2_session* session, const nghttp2_frame* frame,
             const uint8_t* name, size_t namelen, const uint8_t* value,
             size_t valuelen, uint8_t flags, void* user_data);

int OnStreamClose(nghttp2_session* session, int32_t stream_id,
                  uint32_t error_code, void* user_data);

int OnBeginHeaders(nghttp2_session* session, const nghttp2_frame* frame,
                   void* user_data);

int OnDataChunkRecv(nghttp2_session* session, uint8_t flags, int32_t stream_id,
                    const uint8_t* data, size_t len, void* user_data);

}  // namespace recv

// implemented in http_response_writer.cpp
// TODO move implementation to http2_send_callbacks.cpp
namespace send {

// implements
// https://nghttp2.org/documentation/types.html#c.nghttp2_send_data_callback
// nghttp2_session_callbacks_set_send_data_callback
int OnSendDataCallback(nghttp2_session*, nghttp2_frame* frame,
                       const uint8_t* framehd, size_t length,
                       nghttp2_data_source* source, void*);

}  // namespace send

}  // namespace server::http::nghttp2_callbacks

USERVER_NAMESPACE_END
