#pragma once

/// @file userver/server/request/request_context.hpp
/// @brief @copybrief server::request::RequestContext

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <userver/server/request/storage_context.hpp>
#include <userver/utils/any_movable.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/statistics/labels.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace impl {
class InternalRequestContext;
}

/// @brief Stores request-specific data during request processing.
///
/// For example: you can store some data in `HandleRequestThrow()` method
/// and access this data in `GetResponseDataForLogging()` method.
///
/// Use typed tags to store and retrieve per-request custom data.
///
/// The context can be used to pass data between middleware, authentication
/// checkers, handlers, and logging hooks.
///
/// ## Example usage:
///
/// Define a shared tag in a header:
/// @snippet samples/postgres_auth/auth_bearer.hpp  request context tag
///
/// Store the value in an authentication checker:
/// @snippet samples/postgres_auth/auth_bearer.cpp  request context store
///
/// Read it in a handler:
/// @snippet samples/postgres_auth/postgres_service.cpp  request context read
class RequestContext final {
public:
    RequestContext();

    RequestContext(RequestContext&&) noexcept;

    RequestContext(const RequestContext&) = delete;

    ~RequestContext();

    /// @brief Stores data under the tag if no value was previously stored.
    /// @returns Reference to the stored data.
    /// @throws std::runtime_error if data for the tag is already stored.
    template <typename Data>
    Data& SetData(const utils::AnyStorageDataTag<StorageContext, Data>& tag, Data data) USERVER_IMPL_LIFETIME_BOUND;

    /// @brief Emplaces data under the tag if no value was previously stored.
    /// @returns Reference to the stored data.
    /// @throws std::runtime_error if data for the tag is already stored.
    template <typename Data, typename... Args>
    Data& EmplaceData(const utils::AnyStorageDataTag<StorageContext, Data>& tag, Args&&... args)
        USERVER_IMPL_LIFETIME_BOUND;

    /// @returns Stored data for the tag.
    /// @throws std::runtime_error if no data was stored.
    template <typename Data>
    Data& GetData(const utils::AnyStorageDataTag<StorageContext, Data>& tag) USERVER_IMPL_LIFETIME_BOUND;

    /// @returns Read-only access to stored data for the tag.
    /// @throws std::runtime_error if no data was stored.
    template <typename Data>
    const Data& GetData(const utils::AnyStorageDataTag<StorageContext, Data>& tag) const USERVER_IMPL_LIFETIME_BOUND;

    /// @returns Pointer to stored data for the tag or nullptr if absent.
    template <typename Data>
    Data* GetDataOptional(const utils::AnyStorageDataTag<StorageContext, Data>& tag
    ) noexcept USERVER_IMPL_LIFETIME_BOUND;

    /// @returns Read-only pointer to stored data for the tag or nullptr if absent.
    template <typename Data>
    const Data* GetDataOptional(const utils::AnyStorageDataTag<StorageContext, Data>& tag
    ) const noexcept USERVER_IMPL_LIFETIME_BOUND;

    /// @brief Erases data for the tag. Erasing an empty slot is harmless.
    template <typename Data>
    void EraseData(const utils::AnyStorageDataTag<StorageContext, Data>& tag) noexcept;

#ifndef ARCADIA_ROOT
    /// @brief Stores user data if it was not previously stored in this.
    /// @throw std::runtime_error if user data was already stored.
    template <typename Data>
    [[deprecated("Use SetData with a typed tag instead")]] Data& SetUserData(Data data);

    /// @brief Emplaces user data if it was not previously stored in this.
    /// @throw std::runtime_error if user data was already stored.
    template <typename Data, typename... Args>
    [[deprecated("Use EmplaceData with a typed tag instead")]] Data& EmplaceUserData(Args&&... args);

    /// @returns Stored user data
    /// @throws std::runtime_error if no data was stored
    /// @throws std::bad_any_cast if data of different type was stored
    template <typename Data>
    [[deprecated("Use GetData with a typed tag instead")]] Data& GetUserData();

    /// @returns Stored user data
    /// @throws std::runtime_error if no data was stored
    /// @throws std::bad_any_cast if data of different type was stored
    template <typename Data>
    [[deprecated("Use GetData with a typed tag instead")]] const Data& GetUserData() const;

    /// @returns A pointer to data of type Data if it was stored before during
    /// current request processing or nullptr otherwise
    template <typename Data>
    [[deprecated("Use GetDataOptional with a typed tag instead")]] std::remove_reference_t<Data>* GetUserDataOptional();

    /// @returns A pointer to data of type Data if it was stored before during
    /// current request processing or nullptr otherwise
    template <typename Data>
    [[deprecated("Use GetDataOptional with a typed tag instead")]] const std::remove_reference_t<Data>*
    GetUserDataOptional() const;

    /// @brief Erases the user data.
    [[deprecated("Use EraseData with a typed tag instead")]] void EraseUserData() noexcept;

    /// @brief Stores the data with specified name if it was not previously stored
    /// in this.
    /// @throw std::runtime_error if data with such name was already stored.
    template <typename Data>
    [[deprecated("Use SetData with a typed tag instead")]] Data& SetData(std::string name, Data data);

    /// @brief Emplaces the data with specified name if it was not previously
    /// stored in this.
    /// @throw std::runtime_error if data with such name was already stored.
    template <typename Data, typename... Args>
    [[deprecated("Use EmplaceData with a typed tag instead")]] Data& EmplaceData(std::string name, Args&&... args);

    /// @returns Stored data with specified name.
    /// @throws std::runtime_error if no data was stored
    /// @throws std::bad_any_cast if data of different type was stored
    template <typename Data>
    [[deprecated("Use GetData with a typed tag instead")]] Data& GetData(std::string_view name);

    /// @returns Stored data with specified name.
    /// @throws std::runtime_error if no data was stored
    /// @throws std::bad_any_cast if data of different type was stored
    template <typename Data>
    [[deprecated("Use GetData with a typed tag instead")]] const Data& GetData(std::string_view name) const;

    /// @returns Stored data with specified name or nullptr if no data found.
    /// @throws std::bad_any_cast if data of different type was stored.
    template <typename Data>
    [[deprecated("Use GetDataOptional with a typed tag instead")]] std::remove_reference_t<Data>* GetDataOptional(
        std::string_view name
    );

    /// @returns Stored data with specified name or nullptr if no data found.
    /// @throws std::bad_any_cast if data of different type was stored.
    template <typename Data>
    [[deprecated("Use GetDataOptional with a typed tag instead")]] const std::remove_reference_t<Data>* GetDataOptional(
        std::string_view name
    ) const;

    /// @brief Erase data with specified name.
    [[deprecated("Use EraseData with a typed tag instead")]] void EraseData(std::string_view name) noexcept;

#endif

    /// @brief Set the metrics shard (path + labels) for this request.
    /// When set, handler metrics will be accumulated on a new subpath "http.handler.path.*"
    /// and the provided labels would be set to each metric
    /// @note If something (e.g. middleware) interrupts the request before this method is called
    /// then the sharded metrics won't be written
    void SetHandlerMetricsShard(std::string_view path, utils::statistics::LabelsSpan labels);

    // TODO : TAXICOMMON-8252
    impl::InternalRequestContext& GetInternalContext() noexcept;

private:
    utils::AnyStorage<StorageContext>& GetStorageContext() noexcept USERVER_IMPL_LIFETIME_BOUND;
    const utils::AnyStorage<StorageContext>& GetStorageContext() const noexcept USERVER_IMPL_LIFETIME_BOUND;

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

    class Impl;
    static constexpr std::size_t kPimplSize = 120;
    utils::FastPimpl<Impl, kPimplSize, alignof(void*)> impl_;
};

template <typename Data>
Data& RequestContext::SetData(const utils::AnyStorageDataTag<StorageContext, Data>& tag, Data data)
    USERVER_IMPL_LIFETIME_BOUND {
    return EmplaceData(tag, std::move(data));
}

template <typename Data, typename... Args>
Data& RequestContext::EmplaceData(const utils::AnyStorageDataTag<StorageContext, Data>& tag, Args&&... args)
    USERVER_IMPL_LIFETIME_BOUND {
    auto& storage = GetStorageContext();
    if (storage.GetOptional(tag)) {
        throw std::runtime_error("Data for the tag is already stored in RequestContext");
    }
    return storage.Emplace(tag, std::forward<Args>(args)...);
}

template <typename Data>
Data& RequestContext::GetData(const utils::AnyStorageDataTag<StorageContext, Data>& tag) USERVER_IMPL_LIFETIME_BOUND {
    return GetStorageContext().Get(tag);
}

template <typename Data>
const Data& RequestContext::GetData(const utils::AnyStorageDataTag<StorageContext, Data>& tag
) const USERVER_IMPL_LIFETIME_BOUND {
    return GetStorageContext().Get(tag);
}

template <typename Data>
Data* RequestContext::GetDataOptional(const utils::AnyStorageDataTag<StorageContext, Data>& tag
) noexcept USERVER_IMPL_LIFETIME_BOUND {
    return GetStorageContext().GetOptional(tag);
}

template <typename Data>
const Data* RequestContext::GetDataOptional(const utils::AnyStorageDataTag<StorageContext, Data>& tag
) const noexcept USERVER_IMPL_LIFETIME_BOUND {
    return GetStorageContext().GetOptional(tag);
}

template <typename Data>
void RequestContext::EraseData(const utils::AnyStorageDataTag<StorageContext, Data>& tag) noexcept {
    GetStorageContext().Erase(tag);
}

#ifndef ARCADIA_ROOT
template <typename Data>
Data& RequestContext::SetUserData(Data data) {
    static_assert(
        !std::is_const_v<Data>,
        "Data stored in RequestContext is mutable if RequestContext is "
        "not `const`. Remove the `const` from the template parameter "
        "of SetUserData as it makes no sense"
    );
    static_assert(
        !std::is_reference_v<Data>,
        "Data in RequestContext is stored by copy. Remove the reference "
        "from the template parameter of SetUserData as it makes no sense"
    );
    return utils::AnyCast<Data&>(SetUserAnyData(std::move(data)));
}

template <typename Data, typename... Args>
Data& RequestContext::EmplaceUserData(Args&&... args) {
    // NOLINTNEXTLINE(google-readability-casting)
    auto& data = SetUserAnyData(Data(std::forward<Args>(args)...));
    return utils::AnyCast<Data&>(data);
}

template <typename Data>
Data& RequestContext::GetUserData() {
    return utils::AnyCast<Data&>(GetUserAnyData());
}

template <typename Data>
const Data& RequestContext::GetUserData() const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<RequestContext*>(this)->GetUserData<Data>();
}

template <typename Data>
std::remove_reference_t<Data>* RequestContext::GetUserDataOptional() {
    auto* data = GetUserAnyDataOptional();
    return data ? &utils::AnyCast<Data&>(*data) : nullptr;
}

template <typename Data>
const std::remove_reference_t<Data>* RequestContext::GetUserDataOptional() const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<RequestContext*>(this)->GetUserDataOptional<Data>();
}

inline void RequestContext::EraseUserData() noexcept { EraseUserAnyData(); }

template <typename Data>
Data& RequestContext::SetData(std::string name, Data data) {
    static_assert(
        !std::is_const_v<Data>,
        "Data stored in RequestContext is mutable if RequestContext is "
        "not `const`. Remove the `const` from the template parameter "
        "of SetData as it makes no sense"
    );
    static_assert(
        !std::is_reference_v<Data>,
        "Data in RequestContext is stored by copy. Remove the reference "
        "from the template parameter of SetData as it makes no sense"
    );
    return utils::AnyCast<Data&>(SetAnyData(std::move(name), std::move(data)));
}

template <typename Data, typename... Args>
Data& RequestContext::EmplaceData(std::string name, Args&&... args) {
    // NOLINTNEXTLINE(google-readability-casting)
    auto& data = SetAnyData(std::move(name), Data(std::forward<Args>(args)...));
    return utils::AnyCast<Data&>(data);
}

template <typename Data>
Data& RequestContext::GetData(std::string_view name) {
    return utils::AnyCast<Data&>(GetAnyData(name));
}

template <typename Data>
const Data& RequestContext::GetData(std::string_view name) const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<RequestContext*>(this)->GetData<Data>(name);
}

template <typename Data>
std::remove_reference_t<Data>* RequestContext::GetDataOptional(std::string_view name) {
    auto* data = GetAnyDataOptional(name);
    return data ? &utils::AnyCast<Data&>(*data) : nullptr;
}

template <typename Data>
const std::remove_reference_t<Data>* RequestContext::GetDataOptional(std::string_view name) const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return const_cast<RequestContext*>(this)->GetDataOptional<Data>(name);
}

inline void RequestContext::EraseData(std::string_view name) noexcept { EraseAnyData(name); }

#endif

}  // namespace server::request

USERVER_NAMESPACE_END
