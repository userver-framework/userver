#include <userver/storages/sqlite/impl/io/common_binders.hpp>

#include <boost/range/algorithm/copy.hpp>
#include <boost/uuid/uuid.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/impl/statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

// -------------------------- Output Bindings ---------------------------------
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::uint8_t> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::int8_t> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::uint16_t> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::int16_t> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::uint32_t> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::int32_t> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::uint64_t> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::int64_t> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<float> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<double> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::string> val) {
    binds.Extract(pos, val.Get());
}

// These 2 should never be called, so we just leave them unimplemented
void FreestandingBind(OutputBindingsFwd&, std::size_t, ExplicitRef<std::string_view>) {
    UINVARIANT(false, "should be unreachable");
}
void FreestandingBind(OutputBindingsFwd&, std::size_t, ExplicitRef<std::optional<std::string_view>>) {
    UINVARIANT(false, "should be unreachable");
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::vector<uint8_t>> val) {
    binds.Extract(pos, val.Get());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<bool> val) {
    int int_val{};
    binds.Extract(pos, int_val);
    val.Get() = static_cast<bool>(int_val);
}

void FreestandingBind(
    OutputBindingsFwd& binds,
    std::size_t pos,
    ExplicitRef<std::chrono::system_clock::time_point> val
) {
    // TODO: What precision we want here?
    std::int64_t nanos{};
    binds.Extract(pos, nanos);
    val.Get() = std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(nanos))
    );
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<boost::uuids::uuid> val) {
    std::vector<std::uint8_t> blob;
    binds.Extract(pos, blob);
    if (blob.size() != boost::uuids::uuid::static_size()) {
        throw SQLiteException{"Incorrect representation of UUIDv4 in SQLite"};
    }
    boost::range::copy(blob, val.Get().begin());
}

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<formats::json::Value> val) {
    std::string json_str;
    binds.Extract(pos, json_str);
    val.Get() = formats::json::FromString(json_str);
}

bool IsNull(OutputBindingsFwd& binds, std::size_t pos) { return binds.IsNull(pos); }

// --------------------------- Input Bindings ---------------------------------
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::uint8_t> val) {
    binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::int8_t> val) {
    binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::uint16_t> val) {
    binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::int16_t> val) {
    binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::uint32_t> val) {
    binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::int32_t> val) {
    binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::uint64_t> val) {
    binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::int64_t> val) {
    binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<float> val) { binds.Bind(pos, val.Get()); }
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<double> val) {
    binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::string> val) {
    binds.Bind(pos, val.Get());
}
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::string_view> val) {
    binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::vector<std::uint8_t>> val) {
    binds.Bind(pos, val.Get());
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<bool> val) { binds.Bind(pos, val.Get()); }

void FreestandingBind(
    InputBindingsFwd& binds,
    std::size_t pos,
    ExplicitCRef<std::chrono::system_clock::time_point> val
) {
    // TODO: Which precision we want here?
    const auto duration = val.Get().time_since_epoch();
    const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    binds.Bind(pos, static_cast<int64_t>(nanos));
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<boost::uuids::uuid> val) {
    std::vector<std::uint8_t> blob{val.Get().begin(), val.Get().end()};
    binds.Bind(pos, blob);
}

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<formats::json::Value> val) {
    const auto json_str = ToString(val.Get());
    binds.Bind(pos, json_str);
}

void BindNull(InputBindingsFwd& binds, std::size_t pos) { binds.Bind(pos); }

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
