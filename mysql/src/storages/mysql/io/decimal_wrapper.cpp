#include <userver/storages/mysql/io/decimal_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

DecimalWrapper::DecimalWrapper() = default;

std::string DecimalWrapper::GetValue() const { return get_value_cb_(source_); }

void DecimalWrapper::Restore(std::string_view db_representation) const {
  restore_cb_(source_, db_representation);
}

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
