#pragma once

#include <string>

#include <userver/storages/redis/reply_fwd.hpp>
#include <userver/storages/redis/reply_types.hpp>
#include <userver/storages/redis/scan_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

// Mock note:
// Request->
//   RequestDataBase
//
// RequestDataBase <- RequestDataImpl
// RequestDataBase <- MockRequestDataBase <- UserMockRequestData

template <typename ReplyType>
class RequestDataBase {
 public:
  virtual ~RequestDataBase() = default;

  virtual void Wait() = 0;

  virtual ReplyType Get(const std::string& request_description) = 0;

  virtual ReplyPtr GetRaw() = 0;
};

template <ScanTag scan_tag>
class RequestScanDataBase {
 public:
  using ReplyElem = typename ScanReplyElem<scan_tag>::type;

  virtual ~RequestScanDataBase() = default;

  void SetRequestDescription(std::string request_description) {
    request_description_ = std::move(request_description);
  }

  virtual ReplyElem Get() = 0;

  virtual ReplyElem& Current() = 0;

  virtual bool Eof() = 0;

 protected:
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::string request_description_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
