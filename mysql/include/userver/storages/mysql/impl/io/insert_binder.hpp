#pragma once

#include <array>

#include <boost/pfr/core.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/mysql/convert.hpp>
#include <userver/storages/mysql/impl/io/binder_declarations.hpp>
#include <userver/storages/mysql/impl/io/params_binder_base.hpp>
#include <userver/storages/mysql/impl/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

// tidy complains about this being specified not in the outer namespace scope
template <typename Row, bool IsMapped>
struct OwnedMappedRow final {};
template <typename Row>
struct OwnedMappedRow<Row, true> final {
  Row value;
};

class InsertBinderBase : public ParamsBinderBase {
 public:
  explicit InsertBinderBase(std::size_t size);
  ~InsertBinderBase();

  InsertBinderBase(const InsertBinderBase& other) = delete;

 protected:
  void SetBindCallback(void* user_data,
                       void (*param_cb)(void*, void*, std::size_t));

  void UpdateBinds(void* binds_array);
};

template <typename Container, typename MapTo = typename Container::value_type>
class InsertBinder final : public InsertBinderBase {
 public:
  explicit InsertBinder(const Container& container)
      : InsertBinderBase{kColumnsCount},
        container_{container},
        current_row_it_{container_.begin()} {
    static_assert(kColumnsCount != 0, "Rows to insert have zero columns");
    static_assert(meta::kIsSizable<Container>,
                  "Container should be sizeable for batch insertion");

    UASSERT(!container_.empty());

    BindColumn();

    SetBindCallback(this, &BindsRowCallback);
  }

  std::size_t GetRowsCount() const final { return container_.size(); }

 private:
  using Row = MapTo;
  static constexpr std::size_t kColumnsCount = boost::pfr::tuple_size_v<Row>;
  static constexpr bool kIsMapped =
      !std::is_same_v<typename Container::value_type, Row>;

  static void BindsRowCallback(void* user_data, void* binds_array,
                               std::size_t row_number);

  void BindColumn() {
    UpdateCurrentRowPtr();
    boost::pfr::for_each_field(*current_row_ptr_, CurrentRowUpdater{*this});
  }

  void UpdateCurrentRowPtr() {
    if constexpr (kIsMapped) {
      current_row_.value =
          storages::mysql::convert::DoConvert<Row>(*current_row_it_);
      current_row_ptr_ = &current_row_.value;
    } else {
      current_row_ptr_ = &*current_row_it_;
    }
  }

  void UpdateCurrentRow(std::size_t row_number) {
    UASSERT(CheckRowNumber(row_number));
    UASSERT(current_row_it_ != container_.end());

    if (row_number == 0) {
      return; // everything already done at construction
    }

    BindColumn();
  }

  bool CheckRowNumber(std::size_t row_number) {
    if (row_number < max_row_number_seen_ ||
        row_number > max_row_number_seen_ + 1) {
      return false;
    }

    max_row_number_seen_ = std::max(row_number, max_row_number_seen_);
    return true;
  }

  class CurrentRowUpdater final {
   public:
    explicit CurrentRowUpdater(InsertBinder& binder) : binder_{binder} {}

    template <typename Field, std::size_t Index>
    void operator()(const Field& f,
                    std::integral_constant<std::size_t, Index>) {
      GetInputBinder(binder_.GetBinds(), Index, f)();
    }

   private:
    InsertBinder& binder_;
  };

  const Container& container_;

  typename Container::const_iterator current_row_it_;

  OwnedMappedRow<Row, kIsMapped> current_row_;
  const Row* current_row_ptr_;

  std::size_t max_row_number_seen_{0};
};

template <typename Container, typename MapTo>
void InsertBinder<Container, MapTo>::BindsRowCallback(void* user_data,
                                                      void* binds_array,
                                                      std::size_t row_number) {
  auto* self = static_cast<InsertBinder*>(user_data);
  UASSERT(self);

  self->UpdateBinds(binds_array);
  self->UpdateCurrentRow(row_number);

  ++self->current_row_it_;
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
