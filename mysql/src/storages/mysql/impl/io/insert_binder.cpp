#include <userver/storages/mysql/impl/io/insert_binder.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

InsertBinderBase::InsertBinderBase(std::size_t size) : ParamsBinderBase{size} {}

InsertBinderBase::~InsertBinderBase() = default;

void InsertBinderBase::SetBindCallback(void* user_data,
                                       char (*param_cb)(void*, void*,
                                                        std::size_t)) {
  GetBinds().SetUserData(user_data);
  GetBinds().SetParamsCallback(param_cb);
}

void InsertBinderBase::UpdateBinds(void* binds_array) {
  UASSERT(binds_array);

  GetBinds().WrapBinds(binds_array);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
