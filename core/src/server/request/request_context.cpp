#include <userver/server/request/request_context.hpp>

#include <memory>
#include <stdexcept>
#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace server::request {

class RequestContext::Impl final {
 public:
  utils::AnyMovable& SetUserAnyData(utils::AnyMovable&& data);
  utils::AnyMovable& GetUserAnyData();
  utils::AnyMovable* GetUserAnyDataOptional();
  void EraseUserAnyData();

  utils::AnyMovable& SetAnyData(std::string&& name, utils::AnyMovable&& data);
  utils::AnyMovable& GetAnyData(const std::string& name);
  utils::AnyMovable* GetAnyDataOptional(const std::string& name);
  void EraseAnyData(const std::string& name);

 private:
  utils::AnyMovable user_data_;
  std::unordered_map<std::string, utils::AnyMovable> named_datum_;
};

utils::AnyMovable& RequestContext::Impl::SetUserAnyData(
    utils::AnyMovable&& data) {
  if (user_data_.HasValue())
    throw std::runtime_error("Data is already stored in RequestContext");
  user_data_ = std::move(data);
  return user_data_;
}

utils::AnyMovable& RequestContext::Impl::GetUserAnyData() {
  if (!user_data_.HasValue())
    throw std::runtime_error("No data stored in RequestContext");
  return user_data_;
}

utils::AnyMovable* RequestContext::Impl::GetUserAnyDataOptional() {
  if (!user_data_.HasValue()) return nullptr;
  return &user_data_;
}

void RequestContext::Impl::EraseUserAnyData() { user_data_.Reset(); }

utils::AnyMovable& RequestContext::Impl::SetAnyData(std::string&& name,
                                                    utils::AnyMovable&& data) {
  auto res = named_datum_.emplace(std::move(name), std::move(data));
  if (!res.second) {
    throw std::runtime_error("Data with name '" + res.first->first +
                             "' is already registered in RequestContext");
  }
  return res.first->second;
}

utils::AnyMovable& RequestContext::Impl::GetAnyData(const std::string& name) {
  auto it = named_datum_.find(name);
  if (it == named_datum_.end()) {
    throw std::runtime_error("Data with name '" + name +
                             "' is not registered in RequestContext");
  }
  return it->second;
}

utils::AnyMovable* RequestContext::Impl::GetAnyDataOptional(
    const std::string& name) {
  auto it = named_datum_.find(name);
  if (it == named_datum_.end()) return nullptr;
  return &it->second;
}

void RequestContext::Impl::EraseAnyData(const std::string& name) {
  auto it = named_datum_.find(name);
  if (it == named_datum_.end()) return;
  named_datum_.erase(it);
}

RequestContext::RequestContext() = default;

RequestContext::~RequestContext() = default;

utils::AnyMovable& RequestContext::SetUserAnyData(utils::AnyMovable&& data) {
  return impl_->SetUserAnyData(std::move(data));
}

utils::AnyMovable& RequestContext::GetUserAnyData() {
  return impl_->GetUserAnyData();
}

utils::AnyMovable* RequestContext::GetUserAnyDataOptional() {
  return impl_->GetUserAnyDataOptional();
}

void RequestContext::EraseUserAnyData() { return impl_->EraseUserAnyData(); }

utils::AnyMovable& RequestContext::SetAnyData(std::string&& name,
                                              utils::AnyMovable&& data) {
  return impl_->SetAnyData(std::move(name), std::move(data));
}

utils::AnyMovable& RequestContext::GetAnyData(const std::string& name) {
  return impl_->GetAnyData(name);
}

utils::AnyMovable* RequestContext::GetAnyDataOptional(const std::string& name) {
  return impl_->GetAnyDataOptional(name);
}

void RequestContext::EraseAnyData(const std::string& name) {
  return impl_->EraseAnyData(name);
}

}  // namespace server::request

USERVER_NAMESPACE_END
