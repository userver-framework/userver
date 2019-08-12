#pragma once

#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <storages/redis/scan_tag.hpp>

namespace redis {
class Reply;
}  // namespace redis

namespace storages {
namespace redis {

// Mock note:
// Request->
//   RequestDataBase
//
// RequestDataBase <- RequestDataImpl
// RequestDataBase <- MockRequestDataBase <- UserMockRequestData

template <typename Result, typename ReplyType = Result>
class RequestDataBase {
 public:
  virtual ~RequestDataBase() = default;

  virtual void Wait() = 0;

  virtual ReplyType Get(const std::string& request_description = {}) = 0;

  virtual std::shared_ptr<::redis::Reply> GetRaw() = 0;
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
  std::string request_description_;
};

}  // namespace redis
}  // namespace storages
