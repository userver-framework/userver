#pragma once

#include <string>

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
};

}  // namespace redis
}  // namespace storages
