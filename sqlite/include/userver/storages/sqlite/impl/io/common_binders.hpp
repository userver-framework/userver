#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

#include <userver/formats/json_fwd.hpp>

#include <userver/storages/sqlite/impl/binder_fwd.hpp>

namespace boost::uuids {
struct uuid;
}

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

template <typename T>
class ExplicitRef final {
public:
    explicit ExplicitRef(T& ref) : ref_{ref} {}
    explicit ExplicitRef(T&& ref) = delete;

    T& Get() { return ref_; }

private:
    T& ref_;
};

template <typename T>
class ExplicitCRef final {
public:
    static_assert(!std::is_const_v<T>);

    explicit ExplicitCRef(const T& ref) : ref_{ref} {}
    explicit ExplicitCRef(T&& ref) = delete;

    const T& Get() const { return ref_; }

private:
    const T& ref_;
};

// -------------------------- Output Bindings ---------------------------------
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::uint8_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::int8_t> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::uint16_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::int16_t> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::uint32_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::int32_t> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::uint64_t> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::int64_t> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<float> val);
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<double> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::string> val);

// These 2 should never be called, so we just leave them unimplemented
void FreestandingBind(OutputBindingsFwd&, std::size_t, ExplicitRef<std::string_view>);
void FreestandingBind(OutputBindingsFwd&, std::size_t, ExplicitRef<std::optional<std::string_view>>);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::vector<std::uint8_t>> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<bool> val);

void FreestandingBind(
    OutputBindingsFwd& binds,
    std::size_t pos,
    ExplicitRef<std::chrono::system_clock::time_point> val
);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<boost::uuids::uuid> val);

void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<formats::json::Value> val);

bool IsNull(OutputBindingsFwd& binds, std::size_t pos);

template <typename T>
void FreestandingBind(OutputBindingsFwd& binds, std::size_t pos, ExplicitRef<std::optional<T>> val) {
    if (IsNull(binds, pos)) {
        val.Get() = std::nullopt;
    } else {
        if (!val.Get().has_value()) {
            val.Get().emplace();
        }
        storages::sqlite::impl::io::FreestandingBind(binds, pos, ExplicitRef<T>{*val.Get()});
    }
}

template <typename T>
void FreestandingBind(OutputBindingsFwd&, std::size_t, ExplicitRef<T>) {
    static_assert(!sizeof(T), "IO support for the type is not implemented.");
}

// --------------------------- Input Bindings ---------------------------------
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::uint8_t> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::int8_t> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::uint16_t> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::int16_t> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::uint32_t> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::int32_t> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::uint64_t> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::int64_t> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<float> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<double> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::string> val);
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::string_view> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::vector<std::uint8_t>> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<bool> val);

void FreestandingBind(
    InputBindingsFwd& binds,
    std::size_t pos,
    ExplicitCRef<std::chrono::system_clock::time_point> val
);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<boost::uuids::uuid> val);

void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<formats::json::Value> val);

void BindNull(InputBindingsFwd& binds, std::size_t pos);

template <typename T>
void FreestandingBind(InputBindingsFwd& binds, std::size_t pos, ExplicitCRef<std::optional<T>> val) {
    if (val.Get().has_value()) {
        storages::sqlite::impl::io::FreestandingBind(binds, pos, ExplicitCRef<T>{*val.Get()});
    } else {
        storages::sqlite::impl::io::BindNull(binds, pos);
    }
}

template <typename T>
void FreestandingBind(InputBindingsFwd&, std::size_t, ExplicitCRef<T>) {
    static_assert(!sizeof(T), "IO support for the type is not implemented.");
}

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
