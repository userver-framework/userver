#pragma once

/// @file userver/server/request/request_context.hpp
/// @brief @copybrief server::request::RequestContext

#include <string>
#include <string_view>
#include <type_traits>

#include <userver/utils/any_movable.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace impl {
class InternalRequestContext;
}

/// @brief Stores request-specific data during request processing.
///
/// For example: you can store some data in `HandleRequestThrow()` method
/// and access this data in `GetResponseDataForLogging()` method.
class RequestContext final {
 public:
  RequestContext();

  RequestContext(RequestContext&&) noexcept;

  RequestContext(const RequestContext&) = delete;

  ~RequestContext();

  /// @brief Stores user data if it was not previously stored in this.
  /// @throw std::runtime_error if user data was already stored.
  template <typename Data>
  Data& SetUserData(Data data);

  /// @brief Emplaces user data if it was not previously stored in this.
  /// @throw std::runtime_error if user data was already stored.
  template <typename Data, typename... Args>
  Data& EmplaceUserData(Args&&... args);

  /// @returns Stored user data
  /// @throws std::runtime_error if no data was stored
  /// @throws std::bad_any_cast if data of different type was stored
  template <typename Data>
  Data& GetUserData();

  /// @returns Stored user data
  /// @throws std::runtime_error if no data was stored
  /// @throws std::bad_any_cast if data of different type was stored
  template <typename Data>
  const Data& GetUserData() const;

  /// @returns A pointer to data of type Data if it was stored before during
  /// current request processing or nullptr otherwise
  template <typename Data>
  std::remove_reference_t<Data>* GetUserDataOptional();

  /// @returns A pointer to data of type Data if it was stored before during
  /// current request processing or nullptr otherwise
  template <typename Data>
  const std::remove_reference_t<Data>* GetUserDataOptional() const;

  /// @brief Erases the user data.
  void EraseUserData();

  /// @brief Stores the data with specified name if it was not previously stored
  /// in this.
  /// @throw std::runtime_error if data with such name was already stored.
  template <typename Data>
  Data& SetData(std::string name, Data data);

  /// @brief Emplaces the data with specified name if it was not previously
  /// stored in this.
  /// @throw std::runtime_error if data with such name was already stored.
  template <typename Data, typename... Args>
  Data& EmplaceData(std::string name, Args&&... args);

  /// @returns Stored data with specified name.
  /// @throws std::runtime_error if no data was stored
  /// @throws std::bad_any_cast if data of different type was stored
  template <typename Data>
  Data& GetData(std::string_view name);

  /// @returns Stored data with specified name.
  /// @throws std::runtime_error if no data was stored
  /// @throws std::bad_any_cast if data of different type was stored
  template <typename Data>
  const Data& GetData(std::string_view name) const;

  /// @returns Stored data with specified name or nullptr if no data found.
  /// @throws std::bad_any_cast if data of different type was stored.
  template <typename Data>
  std::remove_reference_t<Data>* GetDataOptional(std::string_view name);

  /// @returns Stored data with specified name or nullptr if no data found.
  /// @throws std::bad_any_cast if data of different type was stored.
  template <typename Data>
  const std::remove_reference_t<Data>* GetDataOptional(
      std::string_view name) const;

  /// @brief Erase data with specified name.
  void EraseData(std::string_view name);

  // TODO : TAXICOMMON-8252
  impl::InternalRequestContext& GetInternalContext();

 private:
  utils::AnyMovable& SetUserAnyData(utils::AnyMovable&& data);
  utils::AnyMovable& GetUserAnyData();
  utils::AnyMovable* GetUserAnyDataOptional();
  void EraseUserAnyData();

  utils::AnyMovable& SetAnyData(std::string&& name, utils::AnyMovable&& data);
  utils::AnyMovable& GetAnyData(std::string_view name);
  utils::AnyMovable* GetAnyDataOptional(std::string_view name);
  void EraseAnyData(std::string_view name);

  class Impl;
  static constexpr std::size_t kPimplSize = 112;
  utils::FastPimpl<Impl, kPimplSize, alignof(void*)> impl_;
};

template <typename Data>
Data& RequestContext::SetUserData(Data data) {
  static_assert(!std::is_const_v<Data>,
                "Data stored in RequestContext is mutable if RequestContext is "
                "not `const`. Remove the `const` from the template parameter "
                "of SetUserData as it makes no sense");
  static_assert(
      !std::is_reference_v<Data>,
      "Data in RequestContext is stored by copy. Remove the reference "
      "from the template parameter of SetUserData as it makes no sense");
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
const std::remove_reference_t<Data>* RequestContext::GetUserDataOptional()
    const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<RequestContext*>(this)->GetUserDataOptional<Data>();
}

inline void RequestContext::EraseUserData() { EraseUserAnyData(); }

template <typename Data>
Data& RequestContext::SetData(std::string name, Data data) {
  static_assert(!std::is_const_v<Data>,
                "Data stored in RequestContext is mutable if RequestContext is "
                "not `const`. Remove the `const` from the template parameter "
                "of SetData as it makes no sense");
  static_assert(
      !std::is_reference_v<Data>,
      "Data in RequestContext is stored by copy. Remove the reference "
      "from the template parameter of SetData as it makes no sense");
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
std::remove_reference_t<Data>* RequestContext::GetDataOptional(
    std::string_view name) {
  auto* data = GetAnyDataOptional(name);
  return data ? &utils::AnyCast<Data&>(*data) : nullptr;
}

template <typename Data>
const std::remove_reference_t<Data>* RequestContext::GetDataOptional(
    std::string_view name) const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<RequestContext*>(this)->GetDataOptional<Data>(name);
}

inline void RequestContext::EraseData(std::string_view name) {
  EraseAnyData(name);
}

}  // namespace server::request

USERVER_NAMESPACE_END
