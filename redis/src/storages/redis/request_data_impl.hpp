#pragma once

#include <memory>
#include <string>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/request.hpp>
#include <userver/utils/assert.hpp>

#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/parse_reply.hpp>
#include <userver/storages/redis/request_data_base.hpp>

#include "client_impl.hpp"
#include "scan_reply.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

namespace impl {

void Wait(USERVER_NAMESPACE::redis::Request& request);

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
  RequestDataImplBase(USERVER_NAMESPACE::redis::Request&& request);

  virtual ~RequestDataImplBase();

 protected:
  ReplyPtr GetReply();

  USERVER_NAMESPACE::redis::Request& GetRequest();

 private:
  USERVER_NAMESPACE::redis::Request request_;
};

template <typename Result, typename ReplyType>
class RequestDataImpl final : public RequestDataImplBase,
                              public RequestDataBase<ReplyType> {
 public:
  explicit RequestDataImpl(USERVER_NAMESPACE::redis::Request&& request)
      : RequestDataImplBase(std::move(request)) {}

  void Wait() override { impl::Wait(GetRequest()); }

  ReplyType Get(const std::string& request_description) override {
    auto reply = GetReply();
    return ParseReply<Result, ReplyType>(std::move(reply), request_description);
  }

  ReplyPtr GetRaw() override { return GetReply(); }
};

template <typename Result, typename ReplyType>
class AggregateRequestDataImpl final : public RequestDataBase<ReplyType> {
  using RequestDataPtr = std::unique_ptr<RequestDataBase<ReplyType>>;

 public:
  explicit AggregateRequestDataImpl(std::vector<RequestDataPtr>&& requests)
      : requests_(std::move(requests)) {}

  void Wait() override {
    for (auto& request : requests_) {
      request->Wait();
    }
  }

  ReplyType Get(const std::string& request_description) override {
    std::vector<typename ReplyType::value_type> result;
    for (auto& request : requests_) {
      auto data = request->Get(request_description);
      std::move(data.begin(), data.end(), std::back_inserter(result));
    }
    return result;
  }

  ReplyPtr GetRaw() override {
    UASSERT_MSG(false, "Unsupported");
    return {};
  }

 private:
  std::vector<RequestDataPtr> requests_;
};

template <typename Result, typename ReplyType>
class DummyRequestDataImpl final : public RequestDataBase<ReplyType> {
 public:
  explicit DummyRequestDataImpl(ReplyPtr&& reply) : reply_(std::move(reply)) {}

  void Wait() override {}

  ReplyType Get(const std::string& request_description) override {
    return ParseReply<Result, ReplyType>(std::move(reply_),
                                         request_description);
  }

  ReplyPtr GetRaw() override { return std::move(reply_); }

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
  size_t shard_{-1UL};
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
      command_control_(command_control),
      request_(
          std::make_unique<Request<ScanReply>>(impl::MakeScanRequest<scan_tag>(
              *client_, key_, shard_, {}, options_, command_control_))) {}

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
  if (Eof())
    throw RequestScan::GetAfterEofException(
        "Trying to call Current() after eof");
  UASSERT(reply_);
  UASSERT(reply_keys_index_ < reply_->GetKeys().size());
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
      auto scan_reply_raw = request_->GetRaw();
      command_control_.force_server_id = scan_reply_raw->server_id;
      auto scan_reply = ParseReply<ScanReply>(std::move(scan_reply_raw),
                                              this->request_description_);
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

}  // namespace storages::redis

USERVER_NAMESPACE_END
