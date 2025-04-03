#pragma once

#include <memory>

#include <storages/redis/impl/request.hpp>

#include "request_data_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

template <typename Request>
Request CreateRequest(impl::Request&& request) {
    return Request(
        std::make_unique<RequestDataImpl<typename Request::Result, typename Request::Reply>>(std::move(request))
    );
}

template <typename Request>
Request CreateAggregateRequest(std::vector<impl::Request>&& requests) {
    using ThisRequestDataImpl = RequestDataImpl<typename Request::Result, typename Request::Reply>;
    using ThisAggregateRequestDataImpl = AggregateRequestDataImpl<typename Request::Result, typename Request::Reply>;

    std::vector<std::unique_ptr<RequestDataBase<typename Request::Reply>>> req_data;
    req_data.reserve(requests.size());
    for (auto& request : requests) {
        req_data.push_back(std::make_unique<ThisRequestDataImpl>(std::move(request)));
    }
    return Request(std::make_unique<ThisAggregateRequestDataImpl>(std::move(req_data)));
}

template <typename Request>
Request CreateDummyRequest(ReplyPtr reply) {
    return Request(
        std::make_unique<DummyRequestDataImpl<typename Request::Result, typename Request::Reply>>(std::move(reply))
    );
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
