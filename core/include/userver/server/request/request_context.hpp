#pragma once

/// @file userver/server/request/request_context.hpp
/// @brief @copybrief server::request::RequestContext

#include <string>

#include <userver/compiler/select.hpp>
#include <userver/utils/any_movable.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

/// @brief Stores request-specific data during request processing.
///
/// For example: you can store some data in `HandleRequestThrow()` method
/// and access this data in `GetResponseDataForLogging()` method.
class RequestContext final {
 public:
  RequestContext();
  RequestContext(RequestContext&&) = delete;
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
  Data& GetData(const std::string& name);

  /// @returns Stored data with specified name.
  /// @throws std::runtime_error if no data was stored
  /// @throws std::bad_any_cast if data of different type was stored
  template <typename Data>
  const Data& GetData(const std::string& name) const;

  /// @returns Stored data with specified name or nullptr if no data found.
  /// @throws std::bad_any_cast if data of different type was stored.
  template <typename Data>
  std::remove_reference_t<Data>* GetDataOptional(const std::string& name);

  /// @returns Stored data with specified name or nullptr if no data found.
  /// @throws std::bad_any_cast if data of different type was stored.
  template <typename Data>
  const std::remove_reference_t<Data>* GetDataOptional(
      const std::string& name) const;

  /// @brief Erase data with specified name.
  void EraseData(const std::string& name);

 private:
  class Impl;

  static constexpr std::size_t kPimplSize = compiler::SelectSize()  //
                                                .ForLibCpp32(24)
                                                .ForLibCpp64(48)
                                                .ForLibStdCpp64(64)
                                                .ForLibStdCpp32(32);

  utils::AnyMovable& SetUserAnyData(utils::AnyMovable&& data);
  utils::AnyMovable& GetUserAnyData();
  utils::AnyMovable* GetUserAnyDataOptional();
  void EraseUserAnyData();

  utils::AnyMovable& SetAnyData(std::string&& name, utils::AnyMovable&& data);
  utils::AnyMovable& GetAnyData(const std::string& name);
  utils::AnyMovable* GetAnyDataOptional(const std::string& name);
  void EraseAnyData(const std::string& name);

  utils::FastPimpl<Impl, kPimplSize, alignof(void*), utils::kStrictMatch> impl_;
};

template <typename Data>
Data& RequestContext::SetUserData(Data data) {
  return utils::AnyCast<Data&>(SetUserAnyData(std::move(data)));
}

template <typename Data, typename... Args>
Data& RequestContext::EmplaceUserData(Args&&... args) {
  return utils::AnyCast<Data&>(
      SetUserAnyData(Data(std::forward<Args>(args)...)));
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
  return utils::AnyCast<Data&>(SetAnyData(std::move(name), std::move(data)));
}

template <typename Data, typename... Args>
Data& RequestContext::EmplaceData(std::string name, Args&&... args) {
  return utils::AnyCast<Data&>(
      SetAnyData(std::move(name), Data(std::forward<Args>(args)...)));
}

template <typename Data>
Data& RequestContext::GetData(const std::string& name) {
  return utils::AnyCast<Data&>(GetAnyData(name));
}

template <typename Data>
const Data& RequestContext::GetData(const std::string& name) const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<RequestContext*>(this)->GetData<Data>(name);
}

template <typename Data>
std::remove_reference_t<Data>* RequestContext::GetDataOptional(
    const std::string& name) {
  auto* data = GetAnyDataOptional(name);
  return data ? &utils::AnyCast<Data&>(*data) : nullptr;
}

template <typename Data>
const std::remove_reference_t<Data>* RequestContext::GetDataOptional(
    const std::string& name) const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<RequestContext*>(this)->GetDataOptional<Data>(name);
}

inline void RequestContext::EraseData(const std::string& name) {
  EraseAnyData(name);
}

}  // namespace server::request

USERVER_NAMESPACE_END
