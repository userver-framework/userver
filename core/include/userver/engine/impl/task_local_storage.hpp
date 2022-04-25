#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <typeinfo>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl::task_local {

using Key = std::size_t;

enum class VariableKind { kNormal, kInherited };

class DataBase {
 public:
  using Deleter = void (*)(DataBase&) noexcept;

  void DeleteSelf() noexcept;

 protected:
  explicit DataBase(Deleter deleter);

 private:
  const Deleter deleter_;
};

class NormalDataBase : public DataBase {
 protected:
  template <typename Derived>
  NormalDataBase(std::in_place_type_t<Derived>, Key /*key*/)
      : DataBase([](DataBase& base) noexcept {
          delete &static_cast<Derived&>(base);
        }) {}
};

class InheritedDataBase : public DataBase {
 public:
  void AddRef() noexcept;

  Key GetKey() const noexcept;

 protected:
  template <typename Derived>
  InheritedDataBase(std::in_place_type_t<Derived>, Key key)
      : DataBase([](DataBase& base) noexcept {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
          if (--static_cast<InheritedDataBase&>(base).ref_counter_ == 0) {
            delete &static_cast<Derived&>(base);
          }
        }),
        key_(key) {}

 private:
  std::atomic<std::size_t> ref_counter_{1};
  const Key key_{};
};

template <VariableKind Kind>
using ConditionalDataBase =
    std::conditional_t<Kind == VariableKind::kInherited, InheritedDataBase,
                       NormalDataBase>;

template <typename T, VariableKind Kind>
class DataImpl final : public ConditionalDataBase<Kind> {
  using Base = ConditionalDataBase<Kind>;

 public:
  template <typename... Args>
  explicit DataImpl(Key key, Args&&... args)
      : Base(std::in_place_type<DataImpl>, key),
        variable_(std::forward<Args>(args)...) {}

  T& Get() noexcept { return variable_; }

 private:
  T variable_;
};

[[noreturn]] void ReportVariableNotSet(const std::type_info& type);

class Storage final {
 public:
  Storage();

  Storage(Storage&&) = delete;
  Storage& operator=(Storage&&) = delete;
  ~Storage();

  // Copies pointers to inherited variables from 'other'
  // 'this' must not contain any variables
  void InheritFrom(Storage& other);

  // Moves other's variables into 'this', leaving 'other' in an empty state
  // 'this' must not contain any variables
  void InitializeFrom(Storage&& other) noexcept;

  // Variable accessors must be called with the same T, Kind, key.
  // Otherwise it is UB.
  template <typename T, VariableKind Kind>
  T& GetOrEmplace(Key key) {
    DataBase* const old_data = GetGeneric(key);
    if (!old_data) {
      const bool has_existing_variable = false;
      return DoEmplace<T, Kind>(key, has_existing_variable);
    }
    return static_cast<DataImpl<T, Kind>&>(*old_data).Get();
  }

  template <typename T, VariableKind Kind>
  T* GetOptional(Key key) noexcept {
    DataBase* const data = GetGeneric(key);
    if (!data) return nullptr;
    return &static_cast<DataImpl<T, Kind>&>(*data).Get();
  }

  template <typename T, VariableKind Kind>
  T& Get(Key key) {
    T* const result = GetOptional<T, Kind>(key);
    if (!result) ReportVariableNotSet(typeid(T));
    return *result;
  }

  template <typename T, VariableKind Kind, typename... Args>
  T& Emplace(Key key, Args&&... args) {
    DataBase* const old_data = GetGeneric(key);
    const bool has_existing_variable = old_data != nullptr;
    auto& result = DoEmplace<T, Kind>(key, has_existing_variable,
                                      std::forward<Args>(args)...);
    if (old_data) old_data->DeleteSelf();
    return result;
  }

  template <typename T, VariableKind Kind>
  void Erase(Key key) noexcept {
    static_assert(Kind == VariableKind::kInherited);
    EraseInherited(key);
  }

 private:
  DataBase* GetGeneric(Key key) noexcept;

  void SetGeneric(Key key, NormalDataBase& node, bool has_existing_variable);

  void SetGeneric(Key key, InheritedDataBase& node, bool has_existing_variable);

  void EraseInherited(Key key) noexcept;

  // Provides strong exception guarantee. Does not delete the old data, if any.
  template <typename T, VariableKind Kind, typename... Args>
  T& DoEmplace(Key key, bool has_existing_variable, Args&&... args) {
    auto new_data =
        std::make_unique<DataImpl<T, Kind>>(key, std::forward<Args>(args)...);
    SetGeneric(key, *new_data, has_existing_variable);
    return new_data.release()->Get();
  }

  struct Impl;
  utils::FastPimpl<Impl, 40, 8> impl_;
};

class Variable final {
 public:
  Variable();

  Variable(const Variable&) = delete;
  Variable(Variable&&) = delete;
  Variable& operator=(const Variable&) = delete;
  Variable& operator=(Variable&&) = delete;

  Key GetKey() const noexcept;

 private:
  const Key key_;
};

Storage& GetCurrentStorage() noexcept;

}  // namespace engine::impl::task_local

USERVER_NAMESPACE_END
