#pragma once

/// @file userver/utils/task_inherited_data.hpp
/// @brief Functions to set task specific data that is inherited by tasks,
/// created via utils::Async, utils::CriticalAsync.

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <userver/utils/any_movable.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_pimpl.hpp>

namespace utils {

class NoTaskInheritedDataException : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

namespace impl {

/// `AnyDataDict` can store data of different types by string key.
class AnyDataDict final {
 public:
  AnyDataDict();
  ~AnyDataDict();

  template <typename Data>
  Data& SetData(std::string name, Data&& data);

  template <typename Data, typename... Args>
  Data& EmplaceData(std::string name, Args&&... args);

  /// @returns stored data
  /// @throws `NoTaskInheritedDataException` if no data was stored
  /// @throws `std::bad_any_cast` if data of different type was stored
  /// In debug build `UASSERT` will fail instead of `std::bad_any_cast` throwing
  template <typename Data>
  const Data& GetData(const std::string& name) const;

  /// @returns a pointer to data of type `Data` if it was stored before or
  /// `nullptr` otherwise
  /// @throws `std::bad_any_cast` if data of different type was stored
  /// In debug build `UASSERT` will fail instead of `std::bad_any_cast` throwing
  template <typename Data>
  const Data* GetDataOptional(const std::string& name) const;

  void EraseData(const std::string& name);

 private:
  class Impl;

  static constexpr auto kPimplSize = 56;

  utils::AnyMovable& SetAnyData(std::string&& name, utils::AnyMovable&& data);
  utils::AnyMovable& SetAnyDataShared(
      std::string&& name, std::shared_ptr<utils::AnyMovable>&& data);
  const utils::AnyMovable& GetAnyData(const std::string& name) const;
  const utils::AnyMovable* GetAnyDataOptional(const std::string& name) const;
  void EraseAnyData(const std::string& name);

  utils::FastPimpl<Impl, kPimplSize, 8, false> impl_;
};

template <typename Data>
Data& AnyDataDict::SetData(std::string name, Data&& data) {
  return utils::AnyMovableCast<Data&>(
      SetAnyData(std::move(name), std::forward<Data>(data)));
}

template <typename Data, typename... Args>
Data& AnyDataDict::EmplaceData(std::string name, Args&&... args) {
  return SetData(std::move(name), Data(std::forward<Args>(args)...));
}

template <typename Data>
const Data& AnyDataDict::GetData(const std::string& name) const {
  try {
    return utils::AnyMovableCast<const Data&>(GetAnyData(name));
  } catch (const std::bad_any_cast& ex) {
    UINVARIANT(false, "Requested data with name '" + name +
                          "' of a wrong type: " + ex.what());
  }
}

template <typename Data>
const Data* AnyDataDict::GetDataOptional(const std::string& name) const {
  auto* data = GetAnyDataOptional(name);
  try {
    return data ? &utils::AnyMovableCast<const Data&>(*data) : nullptr;
  } catch (const std::bad_any_cast& ex) {
    UINVARIANT(false, "Requested data with name '" + name +
                          "' of a wrong type: " + ex.what());
  }
}

inline void AnyDataDict::EraseData(const std::string& name) {
  EraseAnyData(name);
}

/// `TaskInheritedDataStorage` object will copy to the child task in
/// `utils::Async`.
/// Contains a shared_ptr to `AnyDataDict`.
/// Makes a copy of data if changes are requested and another task has a
/// shared_ptr to the same data.
class TaskInheritedDataStorage final {
 public:
  template <typename Data>
  Data& SetData(std::string name, Data data) {
    MakeUnique();
    return storage_->SetData(std::move(name), std::forward<Data>(data));
  }

  template <typename Data, typename... Args>
  Data& EmplaceData(std::string name, Args&&... args) {
    MakeUnique();
    return storage_->EmplaceData<Data>(std::move(name),
                                       std::forward<Args>(args)...);
  }

  template <typename Data>
  const Data& GetData(const std::string& name) const {
    CheckNotNull(name);
    return storage_->GetData<Data>(name);
  }

  template <typename Data>
  const Data* GetDataOptional(const std::string& name) const {
    if (!storage_) return nullptr;
    return storage_->GetDataOptional<Data>(name);
  }

  void EraseData(const std::string& name);

 private:
  void CheckNotNull(const std::string& name) const;
  void MakeUnique();

  std::shared_ptr<AnyDataDict> storage_{};
};

TaskInheritedDataStorage& GetTaskInheritedDataStorage();

}  // namespace impl

/// @brief Set or replace a data that could be later retrieved from current or
/// one of the child tasks via utils::GetTaskInheritedData or
/// utils::GetTaskInheritedDataOptional
template <typename Data>
void SetTaskInheritedData(std::string name, Data&& value) {
  impl::GetTaskInheritedDataStorage().SetData(std::move(name),
                                              std::forward<Data>(value));
}

/// @brief Set or replace a data that could be later retrieved from current or
/// one of the child tasks via utils::GetTaskInheritedData or
/// utils::GetTaskInheritedDataOptional
template <typename Data, typename... Args>
void EmplaceTaskInheritedData(std::string name, Args&&... args) {
  impl::GetTaskInheritedDataStorage().EmplaceData<Data>(
      std::move(name), std::forward<Args>(args)...);
}

/// @brief Returns the data, set earlier in current or parent task via
/// utils::SetTaskInheritedData(name)
///
/// @throws utils::NoTaskInheritedDataException if no data with `name` was set
/// @throws if data of different type was stored throws utils::InvariantError
/// in release build or asserts in debug builds
template <typename Data>
const Data& GetTaskInheritedData(const std::string& name) {
  return impl::GetTaskInheritedDataStorage().GetData<Data>(name);
}

/// @brief Returns the data, set earlier in current or parent task via
/// utils::SetTaskInheritedData(name); returns nullptr if no data with `name`
/// was set.
template <typename Data>
const Data* GetTaskInheritedDataOptional(const std::string& name) {
  return impl::GetTaskInheritedDataStorage().GetDataOptional<Data>(name);
}

/// @brief Erase a data so that is could not be retrieved from current task
/// any more
void EraseTaskInheritedData(const std::string& name);

}  // namespace utils
