#include "request_data_impl.hpp"

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

namespace impl {

void Wait(impl::Request& request) {
    try {
        request.Get();
    } catch (const std::exception& ex) {
        LOG_WARNING() << "exception in request.Get(): " << ex;
    }
}

}  // namespace impl

RequestDataImplBase::RequestDataImplBase(impl::Request&& request) : request_(std::move(request)) {}

RequestDataImplBase::~RequestDataImplBase() = default;

ReplyPtr RequestDataImplBase::GetReply() { return request_.Get(); }

impl::Request& RequestDataImplBase::GetRequest() { return request_; }

}  // namespace storages::redis

USERVER_NAMESPACE_END
