#include <userver/storages/sqlite/impl/io/common_binders.hpp>

#include <userver/storages/sqlite/impl/statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

// -------------------------- Output Bindings ---------------------------------
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint8_t> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int8_t> val) {
  binds.Extract(pos, val.Get());
}


void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint16_t> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int16_t> val) {
  binds.Extract(pos, val.Get());
}


void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint32_t> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int32_t> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint64_t> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int64_t> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<float> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<double> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::string> val) {
  binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::vector<uint8_t>> val) {
  binds.Extract(pos, val.Get());
}

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

void BindNull(InputBindingsFwd& binds, std::size_t pos) { binds.Bind(pos); }

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
