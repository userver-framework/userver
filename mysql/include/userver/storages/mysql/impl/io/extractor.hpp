#pragma once

#include <vector>

#include <userver/utils/meta.hpp>

#include <userver/storages/mysql/impl/io/result_binder.hpp>

#include <userver/storages/mysql/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

template <typename Container>
class InPlaceStorage final {
 public:
  explicit InPlaceStorage(ResultBinder& binder);

  void Reserve(std::size_t size);

  template <typename ExtractionTag>
  impl::bindings::OutputBindings& BindNextRow();

  void CommitLastRow();

  void RollbackLastRow();

  Container&& ExtractData();

 private:
  Container data_;
  ResultBinder& binder_;
};

template <typename Container, typename MapFrom>
class ProxyingStorage final {
 public:
  explicit ProxyingStorage(ResultBinder& binder);

  void Reserve(std::size_t size);

  template <typename ExtractionTag>
  impl::bindings::OutputBindings& BindNextRow();

  void CommitLastRow();

  void RollbackLastRow();

  Container&& ExtractData();

 private:
  static constexpr bool kIsMapped =
      !std::is_same_v<typename Container::value_type, MapFrom>;

  MapFrom row_;
  Container data_;
  ResultBinder& binder_;
};

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
  static constexpr std::size_t GetColumnsCount() {
    if constexpr (std::is_same_v<ExtractionTag, RowTag>) {
      return boost::pfr::tuple_size_v<MapFrom>;
    } else if constexpr (std::is_same_v<ExtractionTag, FieldTag>) {
      return 1;
    } else {
      static_assert(!sizeof(ExtractionTag), "should be unreachable");
    }
  }

  static constexpr bool kIsMapped =
      !std::is_same_v<typename Container::value_type, MapFrom>;
  // TODO : add more std:: containers that allow us to emplace/emplace_back and
  // then update the value (list, queue, deque etc)
  static constexpr bool kCanBindInPlace =
      std::is_same_v<Container, std::vector<MapFrom>>;

  // If we are mapped we are out of luck - have to create a MapFrom instance
  // first, and then insert it.
  // If we can't bind in place the same applies.
  // Otherwise, we can emplace_back and bind that new instance.
  using InternalStorageType =
      std::conditional_t<!kIsMapped && kCanBindInPlace,
                         InPlaceStorage<Container>,
                         ProxyingStorage<Container, MapFrom>>;
  InternalStorageType storage_;
};

template <typename Container, typename MapFrom, typename ExtractionTag>
TypedExtractor<Container, MapFrom, ExtractionTag>::TypedExtractor()
    : ExtractorBase{GetColumnsCount()},
      storage_{InternalStorageType{binder_}} {}

template <typename Container, typename MapFrom, typename ExtractionTag>
void TypedExtractor<Container, MapFrom, ExtractionTag>::Reserve(
    std::size_t size) {
  storage_.Reserve(size);
}

template <typename Container, typename MapFrom, typename ExtractionTag>
impl::bindings::OutputBindings&
TypedExtractor<Container, MapFrom, ExtractionTag>::BindNextRow() {
  return storage_.template BindNextRow<ExtractionTag>();
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
  return GetColumnsCount();
}

template <typename Container, typename MapFrom, typename ExtractionTag>
Container&& TypedExtractor<Container, MapFrom, ExtractionTag>::ExtractData() {
  return storage_.ExtractData();
}

template <typename Container>
InPlaceStorage<Container>::InPlaceStorage(ResultBinder& binder)
    : binder_{binder} {}

template <typename Container>
void InPlaceStorage<Container>::Reserve(std::size_t size) {
  // +1 because we over-commit one row and then rollback it in default execution
  // path
  data_.reserve(size + 1);
}

template <typename Container>
template <typename ExtractionTag>
impl::bindings::OutputBindings& InPlaceStorage<Container>::BindNextRow() {
  data_.emplace_back();
  return binder_.BindTo(data_.back(), ExtractionTag{});
}

template <typename Container>
void InPlaceStorage<Container>::CommitLastRow() {
  // no-op, already committed
}

template <typename Container>
void InPlaceStorage<Container>::RollbackLastRow() {
  data_.pop_back();
}

template <typename Container>
Container&& InPlaceStorage<Container>::ExtractData() {
  return std::move(data_);
}

template <typename Container, typename MapFrom>
ProxyingStorage<Container, MapFrom>::ProxyingStorage(ResultBinder& binder)
    : binder_{binder} {}

template <typename Container, typename MapFrom>
void ProxyingStorage<Container, MapFrom>::Reserve(std::size_t size) {
  if constexpr (meta::kIsReservable<Container>) {
    data_.reserve(size);
  }
}

template <typename Container, typename MapFrom>
template <typename ExtractionTag>
impl::bindings::OutputBindings&
ProxyingStorage<Container, MapFrom>::BindNextRow() {
  return binder_.BindTo(row_, ExtractionTag{});
}

template <typename Container, typename MapFrom>
void ProxyingStorage<Container, MapFrom>::CommitLastRow() {
  if constexpr (kIsMapped) {
    *meta::Inserter(data_) =
        storages::mysql::convert::DoConvert<typename Container::value_type>(
            std::move(row_));
  } else {
    *meta::Inserter(data_) = std::move(row_);
  }
}

template <typename Container, typename MapFrom>
void ProxyingStorage<Container, MapFrom>::RollbackLastRow() {
  // no-op, because either this function or commit is called, not both
}

template <typename Container, typename MapFrom>
Container&& ProxyingStorage<Container, MapFrom>::ExtractData() {
  return std::move(data_);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
