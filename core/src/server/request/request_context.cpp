#include <userver/server/request/request_context.hpp>

#include <memory>
#include <stdexcept>

#include <server/request/internal_request_context.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {

struct LegacyStorage final {
    utils::AnyMovable user_data;
    utils::impl::TransparentMap<std::string, utils::AnyMovable> named_data;
};

}  // namespace

class RequestContext::Impl final {
public:
    utils::AnyStorage<StorageContext>& GetStorageContext() noexcept USERVER_IMPL_LIFETIME_BOUND {
        return storage_context_;
    }
    const utils::AnyStorage<StorageContext>& GetStorageContext() const noexcept USERVER_IMPL_LIFETIME_BOUND {
        return storage_context_;
    }

#ifndef ARCADIA_ROOT
    utils::AnyMovable& SetUserAnyData(utils::AnyMovable&& data);
    utils::AnyMovable& GetUserAnyData();
    utils::AnyMovable* GetUserAnyDataOptional() noexcept;
    void EraseUserAnyData() noexcept;

    utils::AnyMovable& SetAnyData(std::string&& name, utils::AnyMovable&& data);
    utils::AnyMovable& GetAnyData(std::string_view name);
    utils::AnyMovable* GetAnyDataOptional(std::string_view name) noexcept;
    void EraseAnyData(std::string_view name) noexcept;

#endif

    impl::InternalRequestContext& GetInternalContext() noexcept;

private:
    utils::AnyStorage<StorageContext> storage_context_;
    impl::InternalRequestContext internal_context_;
#ifndef ARCADIA_ROOT
    LegacyStorage legacy_storage_;
#endif
};

#ifndef ARCADIA_ROOT
utils::AnyMovable& RequestContext::Impl::SetUserAnyData(utils::AnyMovable&& data) {
    auto& user_data = legacy_storage_.user_data;
    if (user_data.HasValue()) {
        throw std::runtime_error("UserData is already stored in RequestContext");
    }
    user_data = std::move(data);
    return user_data;
}

utils::AnyMovable& RequestContext::Impl::GetUserAnyData() {
    auto* data = GetUserAnyDataOptional();
    if (!data) {
        throw std::runtime_error("No data stored in RequestContext");
    }
    return *data;
}

utils::AnyMovable* RequestContext::Impl::GetUserAnyDataOptional() noexcept {
    if (!legacy_storage_.user_data.HasValue()) {
        return nullptr;
    }
    return &legacy_storage_.user_data;
}

void RequestContext::Impl::EraseUserAnyData() noexcept { legacy_storage_.user_data.Reset(); }

utils::AnyMovable& RequestContext::Impl::SetAnyData(std::string&& name, utils::AnyMovable&& data) {
    auto res = legacy_storage_.named_data.emplace(std::move(name), std::move(data));
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
    return utils::FindOrNullptr(legacy_storage_.named_data, name);
}

void RequestContext::Impl::EraseAnyData(std::string_view name) noexcept {
    auto it = legacy_storage_.named_data.find(name);
    if (it == legacy_storage_.named_data.end()) {
        return;
    }
    legacy_storage_.named_data.erase(it);
}

#endif

impl::InternalRequestContext& RequestContext::Impl::GetInternalContext() noexcept { return internal_context_; }

RequestContext::RequestContext() = default;

RequestContext::RequestContext(RequestContext&&) noexcept = default;

RequestContext::~RequestContext() = default;

utils::AnyStorage<StorageContext>& RequestContext::GetStorageContext() noexcept USERVER_IMPL_LIFETIME_BOUND {
    return impl_->GetStorageContext();
}

const utils::AnyStorage<StorageContext>& RequestContext::GetStorageContext() const
    noexcept USERVER_IMPL_LIFETIME_BOUND {
    return impl_->GetStorageContext();
}

#ifndef ARCADIA_ROOT
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

#endif

void RequestContext::SetHandlerMetricsShard(std::string_view path, utils::statistics::LabelsSpan labels) {
    impl_->GetInternalContext().SetHandlerMetricsShard(path, labels);
}

impl::InternalRequestContext& RequestContext::GetInternalContext() noexcept { return impl_->GetInternalContext(); }

}  // namespace server::request

USERVER_NAMESPACE_END
