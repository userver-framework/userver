#include <userver/storages/sqlite/impl/io/common_binders.hpp>

#include <userver/storages/sqlite/impl/statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

// --------------------------- Input Bindings ---------------------------------

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::uint8_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::int8_t> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::uint16_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::int16_t> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::uint32_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::int32_t> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::uint64_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::int64_t> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<float> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<double> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::string> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::string_view> val) {
  binds.Bind(pos, val.Get());
}

void BindNull(InputBindingsFwd& binds, std::size_t pos) {
  // binds.BindNull(pos);
}

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
