#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <boost/pfr/core.hpp>

#include <userver/storages/mysql/impl/binder_fwd.hpp>
#include <userver/storages/mysql/impl/io/binder_declarations.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

class ResultBinder final {
 public:
  explicit ResultBinder(std::size_t size);
  ~ResultBinder();

  ResultBinder(const ResultBinder& other) = delete;
  ResultBinder(ResultBinder&& other) noexcept;

  template <typename T>
  OutputBindingsFwd& BindTo(T& row) {
    boost::pfr::for_each_field(row, FieldBinder{GetBinds()});

    return GetBinds();
  }

  OutputBindingsFwd& GetBinds();

 private:
  class FieldBinder final {
   public:
    explicit FieldBinder(OutputBindingsFwd& binds) : binds_{binds} {}

    template <typename Field, size_t Index>
    void operator()(Field& field,
                    std::integral_constant<std::size_t, Index> i) {
      GetOutputBinder(binds_, i, field)();
    }

   private:
    OutputBindingsFwd& binds_;
  };

  OutputBindingsPimpl impl_;
};

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
