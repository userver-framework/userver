#pragma once

#include "request_data_impl.hpp"
#include "transaction_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class RequestExecDataImpl final : public RequestDataBase<void> {
public:
    RequestExecDataImpl(impl::Request&& request, std::vector<TransactionImpl::ResultPromise>&& result_promises);

    void Wait() override;

    void Get(const std::string& request_description) override;

    ReplyPtr GetRaw() override { return GetReply(); }

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept override {
        UASSERT_MSG(false, "Not implemented");
        return nullptr;
    }

private:
    ReplyPtr GetReply() { return request_.Get(); }

    impl::Request request_;
    std::vector<TransactionImpl::ResultPromise> result_promises_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
