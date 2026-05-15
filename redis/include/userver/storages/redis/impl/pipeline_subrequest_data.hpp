#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <userver/engine/future.hpp>
#include <userver/storages/redis/request_data_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

[[noreturn]] void ThrowPipelineNotStarted(std::string_view description);

template <typename ReplyType>
class PipelineSubrequestDataImpl final : public RequestDataBase<ReplyType> {
public:
    PipelineSubrequestDataImpl(engine::Future<ReplyType> future) : future_(std::move(future)) {}

    void Wait() override { ThrowIfNotReady("Wait() for"); }

    ReplyType Get(const std::string& /*request_description*/) override {
        ThrowIfNotReady("Get()");
        return future_.get();
    }

    ReplyPtr GetRaw() override { throw std::logic_error("call PipelineSubrequestDataImpl::GetRaw()"); }

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept override {
        UASSERT_MSG(false, "Not implemented");
        return nullptr;
    }

private:
    void ThrowIfNotReady(std::string_view description) {
        if (future_.wait_until(engine::Deadline::Passed()) != engine::FutureStatus::kReady) {
            ThrowPipelineNotStarted(description);
        }
    }

    engine::Future<ReplyType> future_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
