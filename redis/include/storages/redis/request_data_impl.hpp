#pragma once

#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <redis/base.hpp>
#include <redis/request.hpp>

#include <storages/redis/reply_types.hpp>
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
  ::redis::ReplyPtr GetReply();

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

  ReplyType Get(const std::string& request_description = {}) override;
};

}  // namespace redis
}  // namespace storages
