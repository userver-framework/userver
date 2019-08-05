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

}  // namespace

namespace impl {
namespace {

::redis::ReplyPtr GetReply(::redis::Request& request) { return request.Get(); }

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

::redis::ReplyPtr RequestDataImplBase::GetReply() {
  return impl::GetReply(request_);
}

::redis::Request& RequestDataImplBase::GetRequest() { return request_; }

RequestScanData::RequestScanData(std::shared_ptr<ClientImpl> client,
                                 size_t shard, ScanOptions options,
                                 const CommandControl& command_control)
    : client_(std::move(client)),
      shard_(shard),
      options_(std::move(options)),
      command_control_(command_control) {
  request_ = std::make_unique<RequestScanImpl>(
      client_->ScanImpl(shard_, {}, options_, command_control_));
}

std::string RequestScanData::Get() {
  if (Eof())
    throw RequestScan::GetAfterEofException("Trying to Get() after eof");
  UASSERT(reply_);
  UASSERT(reply_keys_index_ < reply_->GetKeys().size());
  return std::move(reply_->GetKeys()[reply_keys_index_++]);
}

std::string& RequestScanData::Current() {
  CheckReply();
  return reply_->GetKeys()[reply_keys_index_];
}

bool RequestScanData::Eof() {
  CheckReply();
  return eof_;
}

void RequestScanData::CheckReply() {
  while (!eof_ && (!reply_ || reply_keys_index_ == reply_->GetKeys().size())) {
    if (request_) {
      auto scan_reply = request_->Get(request_description_);
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
      *request_ = client_->ScanImpl(shard_, reply_->GetCursor(), options_,
                                    command_control_);
    else
      request_.reset();
  }
}

}  // namespace redis
}  // namespace storages
