#pragma once

#include <boost/pfr/core.hpp>

#include <userver/storages/mysql/impl/io/insert_binder.hpp>
#include <userver/storages/mysql/impl/io/params_binder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class BindHelper final {
 public:
  template <typename... Args>
  static io::ParamsBinder BindParams(const Args&... args) {
    return io::ParamsBinder::BindParams(args...);
  }

  template <typename T>
  static io::ParamsBinder BindRowAsParams(const T& row) {
    // TODO : reuse DetectIsSuitableRowType from PG
    using Row = std::decay_t<T>;
    static_assert(boost::pfr::tuple_size_v<Row> != 0,
                  "Row to insert has zero columns");

    return std::apply(
        [](const auto&... args) {
          return impl::io::ParamsBinder::BindParams(args...);
        },
        boost::pfr::structure_tie(row));
  }

  template <typename Container>
  static io::InsertBinder<Container> BindContainerAsParams(
      const Container& rows) {
    return io::InsertBinder{rows};
  }

  template <typename MapTo, typename Container>
  static io::InsertBinder<Container, MapTo> BindContainerAsParamsMapped(
      const Container& rows) {
    return io::InsertBinder<Container, MapTo>{rows};
  }
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
