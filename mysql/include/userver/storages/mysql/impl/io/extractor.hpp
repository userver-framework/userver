#pragma once

#include <vector>

#include <userver/storages/mysql/impl/io/result_binder.hpp>
#include <userver/storages/mysql/impl/io/traits.hpp>

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

template <typename Container, typename MapFrom, typename ExtractionTag>
class TypedExtractor final : public ExtractorBase {
 public:
  TypedExtractor();

  void Reserve(std::size_t size) final;

  impl::bindings::OutputBindings& BindNextRow() final;

  void CommitLastRow() final;

  void RollbackLastRow() final;

  std::size_t ColumnsCount() const final;

  Container&& ExtractData();

 private:
  class IdentityStorage final {
   public:
    explicit IdentityStorage(ResultBinder& binder);

    void Reserve(std::size_t size);

    impl::bindings::OutputBindings& BindNextRow();

    void CommitLastRow();

    void RollbackLastRow();

    Container&& ExtractData();

   private:
    Container data_;
    ResultBinder& binder_;
  };

  class MappedStorage final {
   public:
    explicit MappedStorage(ResultBinder& binder);

    void Reserve(std::size_t size);

    impl::bindings::OutputBindings& BindNextRow();

    void CommitLastRow();

    void RollbackLastRow();

    Container&& ExtractData();

   private:
    MapFrom row_;
    Container data_;
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

  using Row = typename Container::value_type;
  static constexpr bool kIsMapped = !std::is_same_v<Row, MapFrom>;
  // TODO : add more std:: containers that allow us to emplace/emplace_back and
  // then update the value (list, queue, deque etc)
  static constexpr bool kCanBindInPlace =
      std::is_same_v<Container, std::vector<MapFrom>>;

  // If we are mapped we are out of luck - have to create a MapFrom instance
  // first, and then insert it.
  // If we can't bind in place the same applies.
  // Otherwise, we can emplace_back and bind that new instance.
  using InternalStorageType =
      std::conditional_t<!kIsMapped && kCanBindInPlace, IdentityStorage,
                         MappedStorage>;
  InternalStorageType storage_;
};

template <typename Container, typename MapFrom, typename ExtractionTag>
TypedExtractor<Container, MapFrom, ExtractionTag>::TypedExtractor()
    : ExtractorBase{kColumnsCount}, storage_{InternalStorageType{binder_}} {}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom, ExtractionTag>::Reserve(
    std::size_t size) {
  storage_.Reserve(size);
}

template <typename Container, typename MapFrom, typename ExtractionTag>
impl::bindings::OutputBindings&
TypedExtractor<Container, MapFrom, ExtractionTag>::BindNextRow() {
  return storage_.BindNextRow();
}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom, ExtractionTag>::CommitLastRow() {
  storage_.CommitLastRow();
}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom, ExtractionTag>::RollbackLastRow() {
  storage_.RollbackLastRow();
}

template <typename Container, typename MapFrom, typename ExtractionTag>
std::size_t TypedExtractor<Container, MapFrom, ExtractionTag>::ColumnsCount()
    const {
  return kColumnsCount;
}

template <typename Container, typename MapFrom, typename ExtractionTag>
Container&& TypedExtractor<Container, MapFrom, ExtractionTag>::ExtractData() {
  return storage_.ExtractData();
}

template <typename Container, typename MapFrom, typename ExtractionTag>
TypedExtractor<Container, MapFrom, ExtractionTag>::IdentityStorage::
    IdentityStorage(ResultBinder& binder)
    : binder_{binder} {}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom,
                    ExtractionTag>::IdentityStorage::Reserve(std::size_t size) {
  // +1 because we overcommit one row and then rollback it in default execution
  // path
  data_.reserve(size + 1);
}

template <typename Container, typename MapFrom, typename ExtractionTag>
impl::bindings::OutputBindings& TypedExtractor<
    Container, MapFrom, ExtractionTag>::IdentityStorage::BindNextRow() {
  data_.emplace_back();
  return binder_.BindTo(data_.back(), ExtractionTag{});
}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom,
                    ExtractionTag>::IdentityStorage::CommitLastRow() {
  // no-op, already committed
}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom,
                    ExtractionTag>::IdentityStorage::RollbackLastRow() {
  data_.pop_back();
}

template <typename Container, typename MapFrom, typename ExtractionTag>
Container&& TypedExtractor<Container, MapFrom,
                           ExtractionTag>::IdentityStorage::ExtractData() {
  return std::move(data_);
}

template <typename Container, typename MapFrom, typename ExtractionTag>
TypedExtractor<Container, MapFrom, ExtractionTag>::MappedStorage::MappedStorage(
    ResultBinder& binder)
    : binder_{binder} {}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom, ExtractionTag>::MappedStorage::Reserve(
    std::size_t size) {
  if constexpr (kIsReservable<Container>) {
    data_.reserve(size);
  }
}

template <typename Container, typename MapFrom, typename ExtractionTag>
impl::bindings::OutputBindings& TypedExtractor<
    Container, MapFrom, ExtractionTag>::MappedStorage::BindNextRow() {
  return binder_.BindTo(row_, ExtractionTag{});
}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom,
                    ExtractionTag>::MappedStorage::CommitLastRow() {
  // TODO : use inserter
  data_.emplace_back(storages::mysql::convert::DoConvert<Row>(std::move(row_)));
}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom,
                    ExtractionTag>::MappedStorage::RollbackLastRow() {
  // no-op, because either this function or commit is called, not both
}

template <typename Container, typename MapFrom, typename ExtractionTag>
Container&& TypedExtractor<Container, MapFrom,
                           ExtractionTag>::MappedStorage::ExtractData() {
  return std::move(data_);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
