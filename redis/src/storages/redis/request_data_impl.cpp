#include "request_data_impl.hpp"

#include <unordered_map>

#include <redis/reply.hpp>
#include <utils/assert.hpp>

#include <storages/redis/client_impl.hpp>

namespace storages {
namespace redis {
namespace {

const std::string kOk{"OK"};
const std::string kPong{"PONG"};

template <ScanTag scan_tag>
Request<ScanReplyTmpl<scan_tag>> MakeScanRequest(
    ClientImpl& client, std::string key, size_t shard,
    typename ScanReplyTmpl<scan_tag>::Cursor cursor,
    ScanOptionsTmpl<scan_tag> options, const CommandControl& command_control) {
  return client.MakeScanRequestWithKey<scan_tag>(
      std::move(key), shard, cursor, std::move(options), command_control);
}

template <>
Request<ScanReply> MakeScanRequest<ScanTag::kScan>(
    ClientImpl& client, std::string, size_t shard, ScanReply::Cursor cursor,
    ScanOptions options, const CommandControl& command_control) {
  return client.MakeScanRequestNoKey(shard, cursor, std::move(options),
                                     command_control);
}

}  // namespace

namespace impl {
namespace {

ReplyPtr GetReply(::redis::Request& request) { return request.Get(); }

}  // namespace

void Wait(::redis::Request& request) {
  try {
    GetReply(request);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in GetReply(): " << ex;
  }
}

}  // namespace impl

RequestDataImplBase::RequestDataImplBase(::redis::Request&& request)
    : request_(std::move(request)) {}

RequestDataImplBase::~RequestDataImplBase() = default;

ReplyPtr RequestDataImplBase::GetReply() { return impl::GetReply(request_); }

::redis::Request& RequestDataImplBase::GetRequest() { return request_; }

template <ScanTag scan_tag>
RequestScanData<scan_tag>::RequestScanData(
    std::shared_ptr<ClientImpl> client, std::string key, size_t shard,
    ScanOptionsTmpl<scan_tag> options, const CommandControl& command_control,
    ScanTag)
    : client_(std::move(client)),
      key_(std::move(key)),
      shard_(shard),
      options_(std::move(options)),
      command_control_(command_control) {
  request_ = std::make_unique<Request<ScanReply>>(MakeScanRequest<scan_tag>(
      *client_, key_, shard_, {}, options_, command_control_));
}

template <ScanTag scan_tag>
typename RequestScanData<scan_tag>::ReplyElem RequestScanData<scan_tag>::Get() {
  if (Eof())
    throw RequestScan::GetAfterEofException("Trying to Get() after eof");
  UASSERT(reply_);
  UASSERT(reply_keys_index_ < reply_->GetKeys().size());
  return std::move(reply_->GetKeys()[reply_keys_index_++]);
}

template <ScanTag scan_tag>
typename RequestScanData<scan_tag>::ReplyElem&
RequestScanData<scan_tag>::Current() {
  CheckReply();
  return reply_->GetKeys()[reply_keys_index_];
}

template <ScanTag scan_tag>
bool RequestScanData<scan_tag>::Eof() {
  CheckReply();
  return eof_;
}

template <ScanTag scan_tag>
void RequestScanData<scan_tag>::CheckReply() {
  while (!eof_ && (!reply_ || reply_keys_index_ == reply_->GetKeys().size())) {
    if (request_) {
      auto scan_reply = request_->Get(this->request_description_);
      if (reply_)
        *reply_ = std::move(scan_reply);
      else
        reply_ = std::make_unique<ScanReply>(std::move(scan_reply));
    } else {
      reply_.reset();
      eof_ = true;
    }
    reply_keys_index_ = 0;

    if (!eof_ && reply_->GetCursor().GetValue())
      *request_ =
          MakeScanRequest<scan_tag>(*client_, key_, shard_, reply_->GetCursor(),
                                    options_, command_control_);
    else
      request_.reset();
  }
}

template class RequestScanData<ScanTag::kScan>;
template class RequestScanData<ScanTag::kSscan>;
template class RequestScanData<ScanTag::kHscan>;
template class RequestScanData<ScanTag::kZscan>;

}  // namespace redis
}  // namespace storages
