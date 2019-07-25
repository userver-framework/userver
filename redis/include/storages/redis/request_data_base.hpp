#pragma once

#include <memory>
#include <string>

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

}  // namespace redis
}  // namespace storages
