#include "http2_session_data.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

Http2SessionData::Http2SessionData(
    const request::HttpRequestConfig& config,
    const HandlerInfoIndex& handler_info_index,
    request::ResponseDataAccounter& data_accounter,
    OnNewRequestCb&& on_new_request_cb)
    : config_(config),
      handler_info_index_(handler_info_index),
      data_accounter_(data_accounter),
      on_new_request_cb_(on_new_request_cb) {}

void Http2SessionData::RegisterStreamData(uint32_t stream_id) {
  // We don't need synchronization because nghttp2 is not thread safe and
  // only one request can be parsed at the same time
  auto [iter, success] = streams_.try_emplace(
      stream_id, config_, handler_info_index_, data_accounter_, stream_id);

  if (!success) {
    // TODO report error
  }

  Http2StreamData& item_ref = iter->second;

  item_ref.constructor.SetHttpMajor(2);
  item_ref.constructor.SetHttpMinor(0);

  // It is guaranteed that referances to std::unordered_map items will not be
  // invalidated
  // https://en.cppreference.com/w/cpp/container/unordered_map/operator_at
  nghttp2_session_set_stream_user_data(session_.GetDataUnsafe().get(),
                                       stream_id, &item_ref);
}

void Http2SessionData::RemoveStreamData(uint32_t stream_id) {
  streams_.erase(stream_id);
}

void Http2SessionData::SubmitEmptyRequest() {
  SubmitRequest(
      HttpRequestConstructor(config_, handler_info_index_, data_accounter_)
          .Finalize());
}

void Http2SessionData::SubmitRequest(
    std::shared_ptr<request::RequestBase>&& request) {
  on_new_request_cb_(std::move(request));
}

concurrent::Variable<impl::SessionPtr>& Http2SessionData::GetSession() {
  return session_;
}

void Http2SessionData::SetSessionPtr(nghttp2_session* session) {
  UASSERT(!session_.GetDataUnsafe());
  session_.GetDataUnsafe() = impl::SessionPtr(session, nghttp2_session_del);
}

}  // namespace server::http

USERVER_NAMESPACE_END
