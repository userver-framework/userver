#pragma once

#include <vector>

#include <userver/storages/mysql/impl/io/result_binder.hpp>

#include <userver/storages/mysql/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

class ExtractorBase {
 public:
  ExtractorBase(std::size_t size);

  virtual void Reserve(std::size_t size) = 0;

  virtual OutputBindingsFwd& BindNextRow() = 0;
  virtual void CommitLastRow() = 0;
  virtual void RollbackLastRow() = 0;

  virtual std::size_t ColumnsCount() const = 0;

  void UpdateBinds(void* binds_array);

 protected:
  ~ExtractorBase();
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  ResultBinder binder_;
};

template <typename T, typename MapFrom = T>
class TypedExtractor final : public ExtractorBase {
 public:
  using StorageType = std::vector<T>;

  TypedExtractor();

  void Reserve(std::size_t size) final;

  impl::bindings::OutputBindings& BindNextRow() final;

  void CommitLastRow() final;

  void RollbackLastRow() final;

  std::size_t ColumnsCount() const final;

  StorageType&& ExtractData();

 private:
  class IdentityStorage final {
   public:
    explicit IdentityStorage(ResultBinder& binder);

    void Reserve(std::size_t size);

    impl::bindings::OutputBindings& BindNextRow();

    void CommitLastRow();

    void RollbackLastRow();

    StorageType&& ExtractData();

   private:
    StorageType data_;
    ResultBinder& binder_;
  };

  class MappedStorage final {
   public:
    explicit MappedStorage(ResultBinder& binder);

    void Reserve(std::size_t size);

    impl::bindings::OutputBindings& BindNextRow();

    void CommitLastRow();

    void RollbackLastRow();

    StorageType&& ExtractData();

   private:
    MapFrom row_;
    StorageType data_;
    ResultBinder& binder_;
  };

  using InternalStorageType =
      std::conditional_t<std::is_same_v<T, MapFrom>, IdentityStorage,
                         MappedStorage>;
  InternalStorageType storage_;
};

template <typename T, typename MapFrom>
TypedExtractor<T, MapFrom>::TypedExtractor()
    : ExtractorBase{boost::pfr::tuple_size_v<MapFrom>},
      storage_{InternalStorageType{binder_}} {}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::Reserve(std::size_t size) {
  storage_.Reserve(size);
}

template <typename T, typename MapFrom>
impl::bindings::OutputBindings& TypedExtractor<T, MapFrom>::BindNextRow() {
  return storage_.BindNextRow();
}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::CommitLastRow() {
  storage_.CommitLastRow();
}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::RollbackLastRow() {
  storage_.RollbackLastRow();
}

template <typename T, typename MapFrom>
std::size_t TypedExtractor<T, MapFrom>::ColumnsCount() const {
  return boost::pfr::tuple_size_v<MapFrom>;
}

template <typename T, typename MapFrom>
typename TypedExtractor<T, MapFrom>::StorageType&&
TypedExtractor<T, MapFrom>::ExtractData() {
  return storage_.ExtractData();
}

template <typename T, typename MapFrom>
TypedExtractor<T, MapFrom>::IdentityStorage::IdentityStorage(
    ResultBinder& binder)
    : binder_{binder} {}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::IdentityStorage::Reserve(std::size_t size) {
  // +1 because we overcommit one row and then rollback it in default execution
  // path
  data_.reserve(size + 1);
}

template <typename T, typename MapFrom>
impl::bindings::OutputBindings&
TypedExtractor<T, MapFrom>::IdentityStorage::BindNextRow() {
  data_.emplace_back();
  return binder_.BindTo(data_.back());
}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::IdentityStorage::CommitLastRow() {
  // no-op, already committed
}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::IdentityStorage::RollbackLastRow() {
  data_.pop_back();
}

template <typename T, typename MapFrom>
typename TypedExtractor<T, MapFrom>::StorageType&&
TypedExtractor<T, MapFrom>::IdentityStorage::ExtractData() {
  return std::move(data_);
}

template <typename T, typename MapFrom>
TypedExtractor<T, MapFrom>::MappedStorage::MappedStorage(ResultBinder& binder)
    : binder_{binder} {}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::MappedStorage::Reserve(std::size_t size) {
  data_.reserve(size);
}

template <typename T, typename MapFrom>
impl::bindings::OutputBindings&
TypedExtractor<T, MapFrom>::MappedStorage::BindNextRow() {
  return binder_.BindTo(row_);
}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::MappedStorage::CommitLastRow() {
  data_.emplace_back(storages::mysql::convert::DoConvert<T>(std::move(row_)));
}

template <typename T, typename MapFrom>
void TypedExtractor<T, MapFrom>::MappedStorage::RollbackLastRow() {
  // no-op, because either this function or commit is called, not both
}

template <typename T, typename MapFrom>
typename TypedExtractor<T, MapFrom>::StorageType&&
TypedExtractor<T, MapFrom>::MappedStorage::ExtractData() {
  return std::move(data_);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
