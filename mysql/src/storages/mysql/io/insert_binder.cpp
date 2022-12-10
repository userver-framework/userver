#include <userver/storages/mysql/io/insert_binder.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>
#include <storages/mysql/impl/mysql_binds_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

InsertBinderBase::InsertBinderBase()
    : impl_(impl::BindsStorage::BindsType::kParameters) {}

InsertBinderBase::~InsertBinderBase() = default;

InsertBinderBase::InsertBinderBase(InsertBinderBase&& other) noexcept = default;

impl::BindsStorage& InsertBinderBase::GetBinds() { return *impl_; }

void InsertBinderBase::SetBindCallback(void* user_data,
                                       void (*param_cb)(void*, void*,
                                                        std::size_t)) {
  impl_->SetBindCallbacks(user_data, param_cb);
}

void InsertBinderBase::PatchBind(void* binds_array, std::size_t pos,
                                 void* buffer, std::size_t* length) {
  auto* binds_ptr = static_cast<MYSQL_BIND*>(binds_array);
  UASSERT(binds_ptr);

  auto& bind = binds_ptr[pos];

  bind.buffer = buffer;
  bind.length = length;
}

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
