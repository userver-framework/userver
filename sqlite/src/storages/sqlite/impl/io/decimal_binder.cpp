#include <userver/storages/sqlite/impl/io/decimal_binders.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/impl/statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, io::DecimalWrapper& val) {
    std::string decimal_str;
    binds.Extract(pos, decimal_str);
    val.Restore(decimal_str);
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, std::optional<io::DecimalWrapper>& val) {
    if (IsNull(binds, pos)) {
        val = std::nullopt;
    } else {
        UASSERT(val.has_value());
        std::string decimal_str;
        binds.Extract(pos, decimal_str);
        val.value().Restore(decimal_str);
    }
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, const io::DecimalWrapper& val) {
    const std::string decimal_str = val.GetValue();
    binds.Bind(pos, decimal_str);
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, const std::optional<io::DecimalWrapper>& val) {
    if (val.has_value()) {
        const std::string decimal_str = val.value().GetValue();
        binds.Bind(pos, decimal_str);
    } else {
        binds.Bind(pos);
    }
}

std::string DecimalWrapper::GetValue() const {
    UASSERT(get_value_cb_);
    return get_value_cb_(source_);
}

void DecimalWrapper::Restore(std::string_view db_representation) const {
    UASSERT(restore_cb_);
    restore_cb_(source_, db_representation);
}

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
