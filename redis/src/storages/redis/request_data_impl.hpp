#pragma once

#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <redis/base.hpp>
#include <redis/request.hpp>

#include <storages/redis/client.hpp>
#include <storages/redis/parse_reply.hpp>
#include <storages/redis/request_data_base.hpp>

namespace storages {
namespace redis {

namespace impl {

void Wait(::redis::Request& request);

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

template <typename Result, typename ReplyType = Result>
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

template <typename Result, typename ReplyType = Result>
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

class ClientImpl;

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

extern template class RequestScanData<ScanTag::kScan>;
extern template class RequestScanData<ScanTag::kSscan>;
extern template class RequestScanData<ScanTag::kHscan>;
extern template class RequestScanData<ScanTag::kZscan>;

}  // namespace redis
}  // namespace storages
