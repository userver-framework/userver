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

template <typename T, typename MapFrom, typename ExtractionTag>
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

  static constexpr std::size_t kColumnsCount = [] {
    if constexpr (std::is_same_v<ExtractionTag, RowTag>) {
      return boost::pfr::tuple_size_v<MapFrom>;
    } else if constexpr (std::is_same_v<ExtractionTag, FieldTag>) {
      return 1;
    } else {
      static_assert(!sizeof(ExtractionTag), "should be unreachable");
    }
  }();

  using InternalStorageType =
      std::conditional_t<std::is_same_v<T, MapFrom>, IdentityStorage,
                         MappedStorage>;
  InternalStorageType storage_;
};

template <typename T, typename MapFrom, typename ExtractionTag>
TypedExtractor<T, MapFrom, ExtractionTag>::TypedExtractor()
    : ExtractorBase{kColumnsCount}, storage_{InternalStorageType{binder_}} {}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom, ExtractionTag>::Reserve(std::size_t size) {
  storage_.Reserve(size);
}

template <typename T, typename MapFrom, typename ExtractionTag>
impl::bindings::OutputBindings&
TypedExtractor<T, MapFrom, ExtractionTag>::BindNextRow() {
  return storage_.BindNextRow();
}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom, ExtractionTag>::CommitLastRow() {
  storage_.CommitLastRow();
}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom, ExtractionTag>::RollbackLastRow() {
  storage_.RollbackLastRow();
}

template <typename T, typename MapFrom, typename ExtractionTag>
std::size_t TypedExtractor<T, MapFrom, ExtractionTag>::ColumnsCount() const {
  return kColumnsCount;
}

template <typename T, typename MapFrom, typename ExtractionTag>
typename TypedExtractor<T, MapFrom, ExtractionTag>::StorageType&&
TypedExtractor<T, MapFrom, ExtractionTag>::ExtractData() {
  return storage_.ExtractData();
}

template <typename T, typename MapFrom, typename ExtractionTag>
TypedExtractor<T, MapFrom, ExtractionTag>::IdentityStorage::IdentityStorage(
    ResultBinder& binder)
    : binder_{binder} {}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom, ExtractionTag>::IdentityStorage::Reserve(
    std::size_t size) {
  // +1 because we overcommit one row and then rollback it in default execution
  // path
  data_.reserve(size + 1);
}

template <typename T, typename MapFrom, typename ExtractionTag>
impl::bindings::OutputBindings&
TypedExtractor<T, MapFrom, ExtractionTag>::IdentityStorage::BindNextRow() {
  data_.emplace_back();
  return binder_.BindTo(data_.back(), ExtractionTag{});
}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom,
                    ExtractionTag>::IdentityStorage::CommitLastRow() {
  // no-op, already committed
}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom,
                    ExtractionTag>::IdentityStorage::RollbackLastRow() {
  data_.pop_back();
}

template <typename T, typename MapFrom, typename ExtractionTag>
typename TypedExtractor<T, MapFrom, ExtractionTag>::StorageType&&
TypedExtractor<T, MapFrom, ExtractionTag>::IdentityStorage::ExtractData() {
  return std::move(data_);
}

template <typename T, typename MapFrom, typename ExtractionTag>
TypedExtractor<T, MapFrom, ExtractionTag>::MappedStorage::MappedStorage(
    ResultBinder& binder)
    : binder_{binder} {}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom, ExtractionTag>::MappedStorage::Reserve(
    std::size_t size) {
  data_.reserve(size);
}

template <typename T, typename MapFrom, typename ExtractionTag>
impl::bindings::OutputBindings&
TypedExtractor<T, MapFrom, ExtractionTag>::MappedStorage::BindNextRow() {
  return binder_.BindTo(row_, ExtractionTag{});
}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom, ExtractionTag>::MappedStorage::CommitLastRow() {
  data_.emplace_back(storages::mysql::convert::DoConvert<T>(std::move(row_)));
}

template <typename T, typename MapFrom, typename ExtractionTag>
void TypedExtractor<T, MapFrom,
                    ExtractionTag>::MappedStorage::RollbackLastRow() {
  // no-op, because either this function or commit is called, not both
}

template <typename T, typename MapFrom, typename ExtractionTag>
typename TypedExtractor<T, MapFrom, ExtractionTag>::StorageType&&
TypedExtractor<T, MapFrom, ExtractionTag>::MappedStorage::ExtractData() {
  return std::move(data_);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
