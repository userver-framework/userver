#include "request_data_impl.hpp"

#include <userver/logging/log.hpp>

namespace storages::redis {

namespace impl {

void Wait(::redis::Request& request) {
  try {
    request.Get();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in request.Get(): " << ex;
  }
}

}  // namespace impl

RequestDataImplBase::RequestDataImplBase(::redis::Request&& request)
    : request_(std::move(request)) {}

RequestDataImplBase::~RequestDataImplBase() = default;

ReplyPtr RequestDataImplBase::GetReply() { return request_.Get(); }

::redis::Request& RequestDataImplBase::GetRequest() { return request_; }

}  // namespace storages::redis
