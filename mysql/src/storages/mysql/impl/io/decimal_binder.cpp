#include <userver/storages/mysql/impl/io/decimal_binder.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>
#include <storages/mysql/impl/bindings/output_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      io::DecimalWrapper& val) {
  binds.Bind(pos, val);
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      std::optional<io::DecimalWrapper>& val) {
  binds.Bind(pos, val);
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      const io::DecimalWrapper& val) {
  binds.Bind(pos, val);
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      const std::optional<io::DecimalWrapper>& val) {
  if (val.has_value()) {
    binds.Bind(pos, *val);
  } else {
    binds.BindNull(pos);
  }
}

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
