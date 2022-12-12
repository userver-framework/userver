#include <userver/storages/mysql/io/insert_binder.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

InsertBinderBase::InsertBinderBase(std::size_t size) : impl_{size} {}

InsertBinderBase::~InsertBinderBase() = default;

InsertBinderBase::InsertBinderBase(InsertBinderBase&& other) noexcept = default;

impl::bindings::InputBindings& InsertBinderBase::GetBinds() { return *impl_; }

void InsertBinderBase::SetBindCallback(void* user_data,
                                       void (*param_cb)(void*, void*,
                                                        std::size_t)) {
  impl_->SetUserData(user_data);
  impl_->SetParamsCallback(param_cb);
}

void InsertBinderBase::PatchBind(void* binds_array, std::size_t pos,
                                 const void* buffer, std::size_t* length) {
  auto* binds_ptr = static_cast<MYSQL_BIND*>(binds_array);
  UASSERT(binds_ptr);

  auto& bind = binds_ptr[pos];

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  bind.buffer = const_cast<void*>(buffer);
  bind.length = length;
}

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
