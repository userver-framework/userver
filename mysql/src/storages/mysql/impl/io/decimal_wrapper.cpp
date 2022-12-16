#include <userver/storages/mysql/impl/io/decimal_binder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

DecimalWrapper::DecimalWrapper() = default;

std::string DecimalWrapper::GetValue() const {
  UASSERT(get_value_cb_);
  return get_value_cb_(source_);
}

void DecimalWrapper::Restore(std::string_view db_representation) const {
  UASSERT(restore_cb_);
  restore_cb_(source_, db_representation);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
