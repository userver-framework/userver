#include "request_data_impl.hpp"

#include <unordered_map>

#include <redis/reply.hpp>

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

}  // namespace redis
}  // namespace storages
