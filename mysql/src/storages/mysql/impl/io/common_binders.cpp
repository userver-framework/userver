#include <userver/storages/mysql/impl/io/common_binders.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>
#include <storages/mysql/impl/bindings/output_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

// -------------------------- Output Bindings ---------------------------------
// yes, we do have to define optional<> variants separately,
// it's not that straightforward for output.
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint8_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::uint8_t>> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int8_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::int8_t>> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint16_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::uint16_t>> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int16_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::int16_t>> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint32_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::uint32_t>> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int32_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::int32_t>> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::uint64_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::uint64_t>> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::int64_t> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::int64_t>> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<float> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<float>> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<double> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<double>> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::string> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<std::string>> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<formats::json::Value> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<formats::json::Value>> val) {
  binds.Bind(pos, val.Get());
}

// These 2 should never be called, so we just leave them unimplemented
void FreestandingBind(OutputBindingsFwd&, std::size_t,
                      ExplicitRef<std::string_view>) {
  UINVARIANT(false, "should be unreachable");
}
void FreestandingBind(OutputBindingsFwd&, std::size_t,
                      ExplicitRef<std::optional<std::string_view>>) {
  UINVARIANT(false, "should be unreachable");
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::chrono::system_clock::time_point> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(
    OutputBindingsFwd& binds, std::size_t pos,
    ExplicitRef<std::optional<std::chrono::system_clock::time_point>> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<Date> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<Date>> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<DateTime> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos,
                      ExplicitRef<std::optional<DateTime>> val) {
  binds.Bind(pos, val.Get());
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
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<formats::json::Value> val) {
  binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<std::chrono::system_clock::time_point> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<Date> val) {
  binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos,
                      ExplicitCRef<DateTime> val) {
  binds.Bind(pos, val.Get());
}

void BindNull(InputBindingsFwd& binds, std::size_t pos) { binds.BindNull(pos); }

// ----------------------- Done with Bindings ---------------------------------

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
