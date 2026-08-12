#include <userver/server/request/request_context.hpp>

#include <memory>
#include <stdexcept>

#include <server/request/internal_request_context.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

class RequestContext::Impl final {
public:
    utils::AnyMovable& SetUserAnyData(utils::AnyMovable&& data);
    utils::AnyMovable& GetUserAnyData();
    utils::AnyMovable* GetUserAnyDataOptional() noexcept;
    void EraseUserAnyData() noexcept;

    utils::AnyMovable& SetAnyData(std::string&& name, utils::AnyMovable&& data);
    utils::AnyMovable& GetAnyData(std::string_view name);
    utils::AnyMovable* GetAnyDataOptional(std::string_view name) noexcept;
    void EraseAnyData(std::string_view name) noexcept;

    impl::InternalRequestContext& GetInternalContext() noexcept;

private:
    utils::AnyMovable user_data_;
    utils::impl::TransparentMap<std::string, utils::AnyMovable> named_datum_;
    impl::InternalRequestContext internal_context_;
};

utils::AnyMovable& RequestContext::Impl::SetUserAnyData(utils::AnyMovable&& data) {
    if (user_data_.HasValue()) {
        throw std::runtime_error("UserData is already stored in RequestContext");
    }
    user_data_ = std::move(data);
    return user_data_;
}

utils::AnyMovable& RequestContext::Impl::GetUserAnyData() {
    if (!user_data_.HasValue()) {
        throw std::runtime_error("No data stored in RequestContext");
    }
    return user_data_;
}

utils::AnyMovable* RequestContext::Impl::GetUserAnyDataOptional() noexcept {
    if (!user_data_.HasValue()) {
        return nullptr;
    }
    return &user_data_;
}

void RequestContext::Impl::EraseUserAnyData() noexcept { user_data_.Reset(); }

utils::AnyMovable& RequestContext::Impl::SetAnyData(std::string&& name, utils::AnyMovable&& data) {
    auto res = named_datum_.emplace(std::move(name), std::move(data));
    if (!res.second) {
        throw std::runtime_error("Data with name '" + res.first->first + "' is already registered in RequestContext");
    }
    return res.first->second;
}

utils::AnyMovable& RequestContext::Impl::GetAnyData(std::string_view name) {
    auto* ptr = GetAnyDataOptional(name);
    if (!ptr) {
        throw std::runtime_error("Data with name '" + std::string{name} + "' is not registered in RequestContext");
    }
    return *ptr;
}

utils::AnyMovable* RequestContext::Impl::GetAnyDataOptional(std::string_view name) noexcept {
    return utils::FindOrNullptr(named_datum_, name);
}

void RequestContext::Impl::EraseAnyData(std::string_view name) noexcept {
    auto it = named_datum_.find(name);
    if (it == named_datum_.end()) {
        return;
    }
    named_datum_.erase(it);
}

impl::InternalRequestContext& RequestContext::Impl::GetInternalContext() noexcept { return internal_context_; }

RequestContext::RequestContext() = default;

RequestContext::RequestContext(RequestContext&&) noexcept = default;

RequestContext::~RequestContext() = default;

utils::AnyMovable& RequestContext::SetUserAnyData(utils::AnyMovable&& data) {
    return impl_->SetUserAnyData(std::move(data));
}

utils::AnyMovable& RequestContext::GetUserAnyData() { return impl_->GetUserAnyData(); }

utils::AnyMovable* RequestContext::GetUserAnyDataOptional() noexcept { return impl_->GetUserAnyDataOptional(); }

void RequestContext::EraseUserAnyData() noexcept { impl_->EraseUserAnyData(); }

utils::AnyMovable& RequestContext::SetAnyData(std::string&& name, utils::AnyMovable&& data) {
    return impl_->SetAnyData(std::move(name), std::move(data));
}

utils::AnyMovable& RequestContext::GetAnyData(std::string_view name) { return impl_->GetAnyData(name); }

utils::AnyMovable* RequestContext::GetAnyDataOptional(std::string_view name) noexcept {
    return impl_->GetAnyDataOptional(name);
}

void RequestContext::EraseAnyData(std::string_view name) noexcept { impl_->EraseAnyData(name); }

void RequestContext::SetHandlerMetricsShard(std::string_view path, utils::statistics::LabelsSpan labels) {
    impl_->GetInternalContext().SetHandlerMetricsShard(path, labels);
}

impl::InternalRequestContext& RequestContext::GetInternalContext() noexcept { return impl_->GetInternalContext(); }

}  // namespace server::request

USERVER_NAMESPACE_END
