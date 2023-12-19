#pragma once

#include <memory>

#include <userver/storages/redis/request.hpp>

#include "request_data_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace impl {

template <typename Result, typename ReplyType = Result>
Request<Result, ReplyType> CreateRequest(
    USERVER_NAMESPACE::redis::Request&& request,
    Request<Result, ReplyType>* /* for ADL */) {
  return Request<Result, ReplyType>(
      std::make_unique<RequestDataImpl<Result, ReplyType>>(std::move(request)));
}

template <typename Result, typename ReplyType = Result>
Request<Result, ReplyType> CreateAggregateRequest(
    std::vector<USERVER_NAMESPACE::redis::Request>&& requests,
    Request<Result, ReplyType>* /* for ADL */) {
  std::vector<std::unique_ptr<RequestDataBase<ReplyType>>> req_data;
  req_data.reserve(requests.size());
  for (auto& request : requests) {
    req_data.push_back(std::make_unique<RequestDataImpl<Result, ReplyType>>(
        std::move(request)));
  }
  return Request<Result, ReplyType>(
      std::make_unique<AggregateRequestDataImpl<Result, ReplyType>>(
          std::move(req_data)));
}

template <typename Result, typename ReplyType = Result>
Request<Result, ReplyType> CreateDummyRequest(
    ReplyPtr&& reply, Request<Result, ReplyType>* /* for ADL */) {
  return Request<Result, ReplyType>(
      std::make_unique<DummyRequestDataImpl<Result, ReplyType>>(
          std::move(reply)));
}

}  // namespace impl

template <typename Request>
Request CreateRequest(USERVER_NAMESPACE::redis::Request&& request) {
  Request* tmp = nullptr;
  return impl::CreateRequest(std::move(request), tmp);
}

template <typename Request>
Request CreateAggregateRequest(
    std::vector<USERVER_NAMESPACE::redis::Request>&& requests) {
  Request* tmp = nullptr;
  return impl::CreateAggregateRequest(std::move(requests), tmp);
}

template <typename Request>
Request CreateDummyRequest(ReplyPtr reply) {
  Request* tmp = nullptr;
  return impl::CreateDummyRequest(std::move(reply), tmp);
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
