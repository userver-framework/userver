#include <server/request/request_context.hpp>

#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace server {
namespace request {

class RequestContext::Impl final {
 public:
  boost::any& SetUserAnyData(boost::any&& data);
  boost::any& GetUserAnyData();
  boost::any* GetUserAnyDataOptional();
  void EraseUserAnyData();

  boost::any& SetAnyData(std::string&& name, boost::any&& data);
  boost::any& GetAnyData(const std::string& name);
  boost::any* GetAnyDataOptional(const std::string& name);
  void EraseAnyData(const std::string& name);

 private:
  std::unique_ptr<boost::any> user_data_;
  std::unordered_map<std::string, boost::any> named_datum_;
};

boost::any& RequestContext::Impl::SetUserAnyData(boost::any&& data) {
  if (user_data_)
    throw std::runtime_error("Data is already stored in RequestContext");
  user_data_ = std::make_unique<boost::any>(std::move(data));
  return *user_data_;
}

boost::any& RequestContext::Impl::GetUserAnyData() {
  if (!user_data_) throw std::runtime_error("No data stored in RequestContext");
  return *user_data_;
}

boost::any* RequestContext::Impl::GetUserAnyDataOptional() {
  if (!user_data_) return nullptr;
  return user_data_.get();
}

void RequestContext::Impl::EraseUserAnyData() { user_data_.reset(); }

boost::any& RequestContext::Impl::SetAnyData(std::string&& name,
                                             boost::any&& data) {
  auto res = named_datum_.emplace(std::move(name), std::move(data));
  if (!res.second) {
    throw std::runtime_error("Data with name '" + res.first->first +
                             "' is already registered in RequestContext");
  }
  return res.first->second;
}

boost::any& RequestContext::Impl::GetAnyData(const std::string& name) {
  auto it = named_datum_.find(name);
  if (it == named_datum_.end()) {
    throw std::runtime_error("Data with name '" + name +
                             "' is not registered in RequestContext");
  }
  return it->second;
}

boost::any* RequestContext::Impl::GetAnyDataOptional(const std::string& name) {
  auto it = named_datum_.find(name);
  if (it == named_datum_.end()) return nullptr;
  return &it->second;
}

void RequestContext::Impl::EraseAnyData(const std::string& name) {
  auto it = named_datum_.find(name);
  if (it == named_datum_.end()) return;
  named_datum_.erase(it);
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
RequestContext::RequestContext() = default;

RequestContext::~RequestContext() = default;

boost::any& RequestContext::SetUserAnyData(boost::any&& data) {
  return impl_->SetUserAnyData(std::move(data));
}

boost::any& RequestContext::GetUserAnyData() { return impl_->GetUserAnyData(); }

boost::any* RequestContext::GetUserAnyDataOptional() {
  return impl_->GetUserAnyDataOptional();
}

void RequestContext::EraseUserAnyData() { return impl_->EraseUserAnyData(); }

boost::any& RequestContext::SetAnyData(std::string&& name, boost::any&& data) {
  return impl_->SetAnyData(std::move(name), std::move(data));
}

boost::any& RequestContext::GetAnyData(const std::string& name) {
  return impl_->GetAnyData(name);
}

boost::any* RequestContext::GetAnyDataOptional(const std::string& name) {
  return impl_->GetAnyDataOptional(name);
}

void RequestContext::EraseAnyData(const std::string& name) {
  return impl_->EraseAnyData(name);
}

}  // namespace request
}  // namespace server
