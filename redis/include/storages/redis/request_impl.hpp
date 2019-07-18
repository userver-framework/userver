#pragma once

#include <memory>

#include <storages/redis/request.hpp>
#include <storages/redis/request_data_impl.hpp>

namespace storages {
namespace redis {
namespace impl {

template <typename Result, typename ReplyType = Result>
Request<Result, ReplyType> CreateRequest(
    ::redis::Request&& request, Request<Result, ReplyType>* /* for ADL */) {
  return Request<Result, ReplyType>(
      std::make_unique<RequestDataImpl<Result, ReplyType>>(std::move(request)));
}

}  // namespace impl

template <typename Request>
Request CreateRequest(::redis::Request&& request) {
  Request* tmp = nullptr;
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  return impl::CreateRequest(std::move(request), tmp);
}

}  // namespace redis
}  // namespace storages
