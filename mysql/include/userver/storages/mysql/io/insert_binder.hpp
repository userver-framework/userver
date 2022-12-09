#pragma once

#include <array>

#include <userver/storages/mysql/io/binder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

template <typename T>
struct RemapStrings final {
  using type = T;

  static type Get(T& t) { return t; }
};

template <>
struct RemapStrings<std::string> final {
  using type = char*;

  static type Get(std::string& s) { return s.data(); }
};

template <typename T>
using RemapStringsT = typename RemapStrings<T>::type;

template <typename T,
          typename Seq = std::make_index_sequence<boost::pfr::tuple_size_v<T>>>
struct TupleOfVectors;

template <typename T, std::size_t... S>
struct TupleOfVectors<T, std::index_sequence<S...>> final {
  using type = std::tuple<
      std::vector<RemapStringsT<boost::pfr::tuple_element_t<S, T>>>...>;
};

template <typename T>
using TupleOfVectorsT = typename TupleOfVectors<T>::type;

class InsertBinderBase : public ParamsBinderBase {
 public:
  InsertBinderBase();
  ~InsertBinderBase();

  InsertBinderBase(const InsertBinderBase& other) = delete;
  InsertBinderBase(InsertBinderBase&& other) noexcept;

  impl::BindsStorage& GetBinds() final;

 protected:
  void FixupBinder(std::size_t pos, char* buffer, std::size_t* lengths);
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  impl::BindsPimpl impl_;
};

template <typename Container>
class InsertBinder final : public InsertBinderBase {
 public:
  using Row = typename Container::value_type;

  explicit InsertBinder(Container& container) : container_{container} {}

  void BindRows() {
    auto& first_row = container_.front();

    boost::pfr::for_each_field(first_row, Initializer{*this});
    boost::pfr::for_each_field(first_row, Mapper{*this});
  }

 private:
  using TupleOfVectors = TupleOfVectorsT<Row>;
  static constexpr std::size_t kColumnsCount = boost::pfr::tuple_size_v<Row>;

  class Initializer final {
   public:
    explicit Initializer(InsertBinder& tester) : tester_{tester} {}

    template <typename Field, std::size_t Index>
    void operator()(Field& first_row_field,
                    std::integral_constant<std::size_t, Index> i) {
      auto& container = tester_.container_;
      auto& columns = tester_.columns_;
      auto& lengths = tester_.lengths_;

      if constexpr (std::is_same_v<Field, std::string>) {
        lengths[i].resize(container.size());
      }
      std::get<Index>(columns).resize(container.size());

      std::size_t offset = 0;
      for (auto& row : container) {
        auto& field = boost::pfr::get<Index>(row);
        std::get<Index>(columns)[offset] = RemapStrings<Field>::Get(field);

        if constexpr (std::is_same_v<Field, std::string>) {
          lengths[i][offset] = field.length();
        }

        ++offset;
      }

      GetBinder(tester_.GetBinds(), i, first_row_field)();
    }

   private:
    InsertBinder& tester_;
  };

  class Mapper final {
   public:
    explicit Mapper(InsertBinder& tester) : tester_{tester} {}

    template <typename Field, std::size_t Index>
    void operator()(Field&, std::integral_constant<std::size_t, Index>) {
      auto& lengths = tester_.lengths_[Index];

      tester_.FixupBinder(
          Index,
          reinterpret_cast<char*>(std::get<Index>(tester_.columns_).data()),
          lengths.empty() ? nullptr : lengths.data());
    }

   private:
    InsertBinder& tester_;
  };

  Container& container_;
  TupleOfVectors columns_;
  std::array<std::vector<std::size_t>, kColumnsCount> lengths_;
};

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
