#pragma once

#include <list>

#include <nghttp2/nghttp2.h>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/server/request/request_config.hpp>

#include "http2_stream_data.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {
using SessionPtr =
    std::unique_ptr<nghttp2_session,
                    std::function<decltype(nghttp2_session_del)>>;
}  // namespace impl

using OnNewRequestCb =
    std::function<void(std::shared_ptr<request::RequestBase>&&)>;

class Http2SessionData final {
 public:
  using Streams = std::unordered_map<uint32_t, Http2StreamData>;

  Http2SessionData(const request::HttpRequestConfig& config,
                   const HandlerInfoIndex& handler_info_index,
                   request::ResponseDataAccounter& data_accounter,
                   OnNewRequestCb&& on_new_request_cb);

  void RegisterStreamData(uint32_t stream_id);

  void RemoveStreamData(uint32_t stream_id);

  void SubmitEmptyRequest();
  void SubmitRequest(std::shared_ptr<request::RequestBase>&& request);

  concurrent::Variable<impl::SessionPtr>& GetSession();

  void SetSessionPtr(nghttp2_session* session);

 private:
  request::HttpRequestConfig config_;
  const HandlerInfoIndex& handler_info_index_;
  request::ResponseDataAccounter& data_accounter_;

  OnNewRequestCb on_new_request_cb_;

  concurrent::Variable<impl::SessionPtr> session_{nullptr};
  Streams streams_{};
};

}  // namespace server::http

USERVER_NAMESPACE_END
