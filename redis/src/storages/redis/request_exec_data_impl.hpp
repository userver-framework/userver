#pragma once

#include "request_data_impl.hpp"
#include "transaction_impl.hpp"

#include <userver/compiler/impl/lifetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class RequestExecDataImpl final : public RequestDataBase<void> {
public:
    RequestExecDataImpl(impl::Request&& request, std::vector<TransactionImpl::ResultPromise>&& result_promises);

    void Wait() noexcept override;

    void Get(const std::string& request_description) override;

    ReplyPtr GetRaw() override { return GetReply(); }

    engine::AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND override {
        UASSERT_MSG(false, "Not implemented");
        return engine::AwaitableToken{};
    }

private:
    ReplyPtr GetReply() { return request_.Get(); }

    impl::Request request_;
    std::vector<TransactionImpl::ResultPromise> result_promises_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
