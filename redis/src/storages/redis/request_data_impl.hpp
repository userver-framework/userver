#pragma once

#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <redis/base.hpp>
#include <redis/request.hpp>
#include <utils/assert.hpp>

#include <storages/redis/client.hpp>
#include <storages/redis/parse_reply.hpp>
#include <storages/redis/request_data_base.hpp>

#include "client_impl.hpp"
#include "scan_reply.hpp"

namespace storages {
namespace redis {

namespace impl {

void Wait(::redis::Request& request);

template <ScanTag scan_tag>
Request<ScanReplyTmpl<scan_tag>> MakeScanRequest(
    ClientImpl& client, std::string key, size_t shard,
    typename ScanReplyTmpl<scan_tag>::Cursor cursor,
    ScanOptionsTmpl<scan_tag> options, const CommandControl& command_control) {
  return client.MakeScanRequestWithKey<scan_tag>(
      std::move(key), shard, cursor, std::move(options), command_control);
}

template <>
inline Request<ScanReply> MakeScanRequest<ScanTag::kScan>(
    ClientImpl& client, std::string, size_t shard, ScanReply::Cursor cursor,
    ScanOptions options, const CommandControl& command_control) {
  return client.MakeScanRequestNoKey(shard, cursor, std::move(options),
                                     command_control);
}

}  // namespace impl

class RequestDataImplBase {
 public:
  RequestDataImplBase(::redis::Request&& request);

  virtual ~RequestDataImplBase();

 protected:
  ReplyPtr GetReply();

  ::redis::Request& GetRequest();

 private:
  ::redis::Request request_;
};

template <typename Result, typename ReplyType>
class RequestDataImpl final : public RequestDataImplBase,
                              public RequestDataBase<Result, ReplyType> {
 public:
  explicit RequestDataImpl(::redis::Request&& request)
      : RequestDataImplBase(std::move(request)) {}

  void Wait() override { impl::Wait(GetRequest()); }

  ReplyType Get(const std::string& request_description = {}) override {
    auto reply = GetReply();
    return ParseReply<Result, ReplyType>(reply, request_description);
  }

  ReplyPtr GetRaw() override { return GetReply(); }
};

template <typename Result, typename ReplyType>
class DummyRequestDataImpl final : public RequestDataBase<Result, ReplyType> {
 public:
  explicit DummyRequestDataImpl(ReplyPtr&& reply) : reply_(std::move(reply)) {}

  void Wait() override {}

  ReplyType Get(const std::string& request_description = {}) override {
    return ParseReply<Result, ReplyType>(reply_, request_description);
  }

  ReplyPtr GetRaw() override { return reply_; }

 private:
  ReplyPtr reply_;
};

template <ScanTag scan_tag>
class RequestScanData final : public RequestScanDataBase<scan_tag> {
 public:
  using ReplyElem = typename ScanReplyElem<scan_tag>::type;

  template <ScanTag scan_tag_param = scan_tag>
  RequestScanData(std::shared_ptr<ClientImpl> client, size_t shard,
                  ScanOptionsTmpl<scan_tag> options,
                  const CommandControl& command_control,
                  std::enable_if_t<scan_tag_param == ScanTag::kScan>* = nullptr)
      : RequestScanData(std::move(client), {}, shard, std::move(options),
                        command_control, scan_tag) {}

  template <ScanTag scan_tag_param = scan_tag>
  RequestScanData(std::shared_ptr<ClientImpl> client, std::string key,
                  size_t shard, ScanOptionsTmpl<scan_tag> options,
                  const CommandControl& command_control,
                  std::enable_if_t<scan_tag_param != ScanTag::kScan>* = nullptr)
      : RequestScanData(std::move(client), std::move(key), shard,
                        std::move(options), command_control, scan_tag) {}

  ReplyElem Get() override;

  ReplyElem& Current() override;

  bool Eof() override;

 private:
  RequestScanData(std::shared_ptr<ClientImpl> client, std::string key,
                  size_t shard, ScanOptionsTmpl<scan_tag> options,
                  const CommandControl& command_control, ScanTag);

  void CheckReply();

  std::shared_ptr<ClientImpl> client_;
  std::string key_;
  size_t shard_;
  ScanOptionsTmpl<scan_tag> options_;
  CommandControl command_control_;

  using ScanReply = ScanReplyTmpl<scan_tag>;

  std::unique_ptr<Request<ScanReply>> request_;
  std::unique_ptr<ScanReply> reply_;
  size_t reply_keys_index_{0};
  bool eof_{false};
};

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
  request_ =
      std::make_unique<Request<ScanReply>>(impl::MakeScanRequest<scan_tag>(
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
      *request_ = impl::MakeScanRequest<scan_tag>(*client_, key_, shard_,
                                                  reply_->GetCursor(), options_,
                                                  command_control_);
    else
      request_.reset();
  }
}

}  // namespace redis
}  // namespace storages
