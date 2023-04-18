#pragma once

#include <userver/storages/mysql/impl/binder_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

class ParamsBinderBase {
 public:
  explicit ParamsBinderBase(std::size_t size);

  ParamsBinderBase(const ParamsBinderBase& other) = delete;
  ParamsBinderBase(ParamsBinderBase&& other) noexcept;

  InputBindingsFwd& GetBinds();

  virtual std::size_t GetRowsCount() const = 0;

 protected:
  ~ParamsBinderBase();

 private:
  InputBindingsPimpl binds_impl_;
};

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
