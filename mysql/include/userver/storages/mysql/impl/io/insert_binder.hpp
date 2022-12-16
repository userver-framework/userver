#pragma once

#include <array>

#include <boost/pfr/core.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/mysql/impl/io/binder_declarations.hpp>
#include <userver/storages/mysql/impl/io/params_binder_base.hpp>
#include <userver/storages/mysql/impl/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

class InsertBinderBase : public ParamsBinderBase {
 public:
  explicit InsertBinderBase(std::size_t size);
  ~InsertBinderBase();

  InsertBinderBase(const InsertBinderBase& other) = delete;
  InsertBinderBase(InsertBinderBase&& other) noexcept;

  void SetBindCallback(void* user_data,
                       void (*param_cb)(void*, void*, std::size_t));

 protected:
  static void PatchBind(void* binds_array, std::size_t pos, const void* buffer,
                        std::size_t* length);
};

template <typename Container>
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
    BindColumns();
  }

  std::size_t GetRowsCount() const final { return container_.size(); }

 private:
  using Row = typename Container::value_type;
  static constexpr std::size_t kColumnsCount = boost::pfr::tuple_size_v<Row>;

  static void BindsRowCallback(void* user_data, void* binds_array,
                               std::size_t row_number);

  void BindColumns() {
    auto& first_row = *container_.begin();
    boost::pfr::for_each_field(first_row, BindsInitializer{*this});

    SetBindCallback(this, &BindsRowCallback);
  }

  void UpdateCurrentRow(std::size_t row_number) {
    UASSERT(CheckRowNumber(row_number));
    UASSERT(current_row_it_ != container_.end());

    boost::pfr::for_each_field(*current_row_it_, CurrentRowUpdater{*this});
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
      if constexpr (std::is_same_v<Field, std::string> ||
                    std::is_same_v<Field, std::string_view>) {
        binder_.current_row_buffers_[Index] = {f.data(), f.length()};
      } else {
        binder_.current_row_buffers_[Index] = {&f, 0};
      }
    }

   private:
    InsertBinder& binder_;
  };

  class BindsInitializer final {
   public:
    explicit BindsInitializer(InsertBinder& binder) : binder_{binder} {}

    template <typename Field, std::size_t Index>
    void operator()(Field& f, std::integral_constant<std::size_t, Index>) {
      GetInputBinder(binder_.GetBinds(), Index, f)();
    }

   private:
    InsertBinder& binder_;
  };

  struct FieldBuffer final {
    const void* data{nullptr};
    std::size_t length{0};
  };

  const Container& container_;

  typename Container::const_iterator current_row_it_;
  std::array<FieldBuffer, kColumnsCount> current_row_buffers_{};

  std::size_t max_row_number_seen_{0};
};

template <typename Container>
void InsertBinder<Container>::BindsRowCallback(void* user_data,
                                               void* binds_array,
                                               std::size_t row_number) {
  auto* self = static_cast<InsertBinder*>(user_data);
  UASSERT(self);

  self->UpdateCurrentRow(row_number);
  for (std::size_t i = 0; i < kColumnsCount; ++i) {
    self->PatchBind(binds_array, i, self->current_row_buffers_[i].data,
                    &self->current_row_buffers_[i].length);
  }

  ++self->current_row_it_;
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
