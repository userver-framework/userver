#pragma once

#include <cstdint>
#include <optional>

#include <boost/pfr/core.hpp>

#include <userver/storages/mysql/impl/binder_fwd.hpp>
#include <userver/storages/mysql/impl/io/binder_declarations.hpp>
#include <userver/storages/mysql/row_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

class ResultBinder final {
 public:
  explicit ResultBinder(std::size_t size);
  ~ResultBinder();

  ResultBinder(const ResultBinder& other) = delete;
  ResultBinder(ResultBinder&& other) noexcept;

  template <typename T, typename ExtractionTag>
  OutputBindingsFwd& BindTo(T& row, ExtractionTag) {
    if constexpr (std::is_same_v<ExtractionTag, RowTag>) {
      boost::pfr::for_each_field(
          row, [&binds = GetBinds()](auto& field, std::size_t i) {
            storages::mysql::impl::io::BindOutput(binds, i, field);
          });
    } else {
      static_assert(std::is_same_v<ExtractionTag, FieldTag>);
      storages::mysql::impl::io::BindOutput(GetBinds(), 0, row);
    }

    return GetBinds();
  }

  OutputBindingsFwd& GetBinds();

 private:
  OutputBindingsPimpl impl_;
};

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
