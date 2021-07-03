#pragma once

/// @file server/request/request_context.hpp

#include <string>

#include <utils/any_movable.hpp>
#include <utils/fast_pimpl.hpp>

namespace server::request {

/// It can store request-specific data during request processing.
/// For example: you can store some data in `HandleRequestThrow()` method
/// and access this data in `GetResponseDataForLogging()` method.
class RequestContext final {
 public:
  RequestContext();
  RequestContext(RequestContext&&) = delete;
  RequestContext(const RequestContext&) = delete;

  ~RequestContext();

  template <typename Data>
  Data& SetUserData(Data data);

  template <typename Data, typename... Args>
  Data& EmplaceUserData(Args&&... args);

  /// @returns stored data
  /// @throws std::runtime_error if no data was stored
  /// @throws std::bad_any_cast if data of different type was stored
  template <typename Data>
  Data& GetUserData();

  /// @returns stored data
  /// @throws std::runtime_error if no data was stored
  /// @throws std::bad_any_cast if data of different type was stored
  template <typename Data>
  const Data& GetUserData() const;

  /// @returns a pointer to data of type Data if it was stored before during
  /// current request processing or nullptr otherwise
  template <typename Data>
  std::remove_reference_t<Data>* GetUserDataOptional();

  /// @returns a pointer to data of type Data if it was stored before during
  /// current request processing or nullptr otherwise
  template <typename Data>
  const std::remove_reference_t<Data>* GetUserDataOptional() const;

  void EraseUserData();

  template <typename Data>
  Data& SetData(std::string name, Data data);

  template <typename Data, typename... Args>
  Data& EmplaceData(std::string name, Args&&... args);

  template <typename Data>
  Data& GetData(const std::string& name);

  template <typename Data>
  const Data& GetData(const std::string& name) const;

  template <typename Data>
  std::remove_reference_t<Data>* GetDataOptional(const std::string& name);

  template <typename Data>
  const std::remove_reference_t<Data>* GetDataOptional(
      const std::string& name) const;

  void EraseData(const std::string& name);

 private:
  class Impl;

#ifdef _LIBCPP_VERSION  // TODO Find out a nicer way to calculate this size
  static constexpr auto kPimplSize = 40 + 8;
#else
  static constexpr auto kPimplSize = 56 + 8;
#endif

  utils::AnyMovable& SetUserAnyData(utils::AnyMovable&& data);
  utils::AnyMovable& GetUserAnyData();
  utils::AnyMovable* GetUserAnyDataOptional();
  void EraseUserAnyData();

  utils::AnyMovable& SetAnyData(std::string&& name, utils::AnyMovable&& data);
  utils::AnyMovable& GetAnyData(const std::string& name);
  utils::AnyMovable* GetAnyDataOptional(const std::string& name);
  void EraseAnyData(const std::string& name);

  utils::FastPimpl<Impl, kPimplSize, 8, true> impl_;
};

template <typename Data>
Data& RequestContext::SetUserData(Data data) {
  return utils::AnyMovableCast<Data&>(SetUserAnyData(std::move(data)));
}

template <typename Data, typename... Args>
Data& RequestContext::EmplaceUserData(Args&&... args) {
  return utils::AnyMovableCast<Data&>(
      SetUserAnyData(Data(std::forward<Args>(args)...)));
}

template <typename Data>
Data& RequestContext::GetUserData() {
  return utils::AnyMovableCast<Data&>(GetUserAnyData());
}

template <typename Data>
const Data& RequestContext::GetUserData() const {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<RequestContext*>(this)->GetUserData<Data>();
}

template <typename Data>
std::remove_reference_t<Data>* RequestContext::GetUserDataOptional() {
  auto* data = GetUserAnyDataOptional();
  return data ? &utils::AnyMovableCast<Data&>(*data) : nullptr;
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
  return utils::AnyMovableCast<Data&>(
      SetAnyData(std::move(name), std::move(data)));
}

template <typename Data, typename... Args>
Data& RequestContext::EmplaceData(std::string name, Args&&... args) {
  return utils::AnyMovableCast<Data&>(
      SetAnyData(std::move(name), Data(std::forward<Args>(args)...)));
}

template <typename Data>
Data& RequestContext::GetData(const std::string& name) {
  return utils::AnyMovableCast<Data&>(GetAnyData(name));
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
  return data ? &utils::AnyMovableCast<Data&>(*data) : nullptr;
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
