#include <utils/task_inherited_data.hpp>

#include <stdexcept>
#include <unordered_map>

#include <engine/task/local_variable.hpp>
#include <utils/assert.hpp>

namespace utils {

namespace impl {

class AnyDataDict::Impl final {
 public:
  utils::AnyMovable& SetAnyData(std::string&& name, utils::AnyMovable&& data);
  utils::AnyMovable& SetAnyDataShared(
      std::string&& name, std::shared_ptr<utils::AnyMovable>&& data);
  const utils::AnyMovable& GetAnyData(const std::string& name) const;
  const utils::AnyMovable* GetAnyDataOptional(const std::string& name) const;
  void EraseAnyData(const std::string& name);

 private:
  std::unordered_map<std::string, std::shared_ptr<utils::AnyMovable>>
      named_data_;
};

utils::AnyMovable& AnyDataDict::Impl::SetAnyData(std::string&& name,
                                                 utils::AnyMovable&& data) {
  auto shared_data = std::make_shared<utils::AnyMovable>(std::move(data));
  auto res = named_data_.try_emplace(std::move(name), std::move(shared_data));
  if (!res.second) res.first->second = std::move(shared_data);
  return *res.first->second;
}

utils::AnyMovable& AnyDataDict::Impl::SetAnyDataShared(
    std::string&& name, std::shared_ptr<utils::AnyMovable>&& data) {
  UASSERT(data);
  auto res = named_data_.try_emplace(std::move(name), std::move(data));
  if (!res.second) res.first->second = std::move(data);
  return *res.first->second;
}

const utils::AnyMovable& AnyDataDict::Impl::GetAnyData(
    const std::string& name) const {
  auto it = named_data_.find(name);
  if (it == named_data_.end()) {
    throw NoTaskInheritedDataException("Data with name '" + name +
                                       "' is not registered in AnyDataDict");
  }
  UASSERT(it->second);
  return *it->second;
}

const utils::AnyMovable* AnyDataDict::Impl::GetAnyDataOptional(
    const std::string& name) const {
  auto it = named_data_.find(name);
  if (it == named_data_.end()) return nullptr;
  UASSERT(it->second);
  return &*it->second;
}

void AnyDataDict::Impl::EraseAnyData(const std::string& name) {
  auto it = named_data_.find(name);
  if (it == named_data_.end()) return;
  named_data_.erase(it);
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
AnyDataDict::AnyDataDict() = default;

AnyDataDict::~AnyDataDict() = default;

utils::AnyMovable& AnyDataDict::SetAnyData(std::string&& name,
                                           utils::AnyMovable&& data) {
  return impl_->SetAnyData(std::move(name), std::move(data));
}

utils::AnyMovable& AnyDataDict::SetAnyDataShared(
    std::string&& name, std::shared_ptr<utils::AnyMovable>&& data) {
  return impl_->SetAnyDataShared(std::move(name), std::move(data));
}

const utils::AnyMovable& AnyDataDict::GetAnyData(
    const std::string& name) const {
  return impl_->GetAnyData(name);
}

const utils::AnyMovable* AnyDataDict::GetAnyDataOptional(
    const std::string& name) const {
  return impl_->GetAnyDataOptional(name);
}

void AnyDataDict::EraseAnyData(const std::string& name) {
  return impl_->EraseAnyData(name);
}

void TaskInheritedDataStorage::CheckNotNull(const std::string& name) const {
  if (!storage_)
    throw NoTaskInheritedDataException("Data with name '" + name +
                                       "' is not registered in AnyDataDict");
}

void TaskInheritedDataStorage::MakeUnique() {
  if (!storage_) {
    storage_ = std::make_shared<AnyDataDict>();
    return;
  }
  if (storage_.use_count() == 1) return;
  storage_ = std::make_shared<AnyDataDict>(*storage_);
}

engine::TaskLocalVariable<TaskInheritedDataStorage>
    task_inherited_data_storage_;

void TaskInheritedDataStorage::EraseData(const std::string& name) {
  if (!storage_) return;
  MakeUnique();
  storage_->EraseData(name);
}

TaskInheritedDataStorage& GetTaskInheritedDataStorage() {
  return *task_inherited_data_storage_;
}

}  // namespace impl

void EraseTaskInheritedData(const std::string& name) {
  return impl::GetTaskInheritedDataStorage().EraseData(name);
}

}  // namespace utils
