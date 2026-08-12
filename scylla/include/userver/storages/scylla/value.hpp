#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

struct Uuid {
    std::uint64_t time_and_version{0};
    std::uint64_t clock_seq_and_node{0};

    static Uuid Random();
    static Uuid TimeBased();
    static Uuid FromString(std::string_view text);

    std::string ToString() const;

    friend bool operator==(const Uuid& a, const Uuid& b) noexcept {
        return a.time_and_version == b.time_and_version && a.clock_seq_and_node == b.clock_seq_and_node;
    }
    friend bool operator!=(const Uuid& a, const Uuid& b) noexcept { return !(a == b); }
};

using Timestamp = std::chrono::system_clock::time_point;

struct Date {
    std::uint32_t days_since_epoch{0};
    friend bool operator==(const Date& a, const Date& b) noexcept { return a.days_since_epoch == b.days_since_epoch; }
};

struct Time {
    std::int64_t nanoseconds_of_day{0};
    friend bool operator==(const Time& a, const Time& b) noexcept {
        return a.nanoseconds_of_day == b.nanoseconds_of_day;
    }
};

using Blob = std::vector<std::byte>;

struct Inet {
    std::string text;

    Inet() = default;
    explicit Inet(std::string t)
        : text(std::move(t))
    {}

    friend bool operator==(const Inet& a, const Inet& b) noexcept { return a.text == b.text; }
};

struct List;
struct Map;
struct Set;

class Value {
public:
    using Storage = std::variant<
        std::monostate,
        std::string,
        std::int8_t,
        std::int16_t,
        std::int32_t,
        std::int64_t,
        bool,
        float,
        double,
        Uuid,
        Timestamp,
        Date,
        Time,
        Blob,
        Inet,
        std::shared_ptr<List>,
        std::shared_ptr<Map>,
        std::shared_ptr<Set>>;

    Value() noexcept = default;
    Value(std::nullptr_t) noexcept {}
    Value(std::monostate) noexcept {}

    Value(std::string v)
        : storage_(std::move(v))
    {}
    Value(const char* v)
        : storage_(std::string{v})
    {}
    Value(std::string_view v)
        : storage_(std::string{v})
    {}

    Value(bool v)
        : storage_(v)
    {}
    Value(std::int8_t v)
        : storage_(v)
    {}
    Value(std::int16_t v)
        : storage_(v)
    {}
    Value(std::int32_t v)
        : storage_(v)
    {}
    Value(std::int64_t v)
        : storage_(v)
    {}
    Value(float v)
        : storage_(v)
    {}
    Value(double v)
        : storage_(v)
    {}

    Value(Uuid v)
        : storage_(v)
    {}
    Value(Timestamp v)
        : storage_(v)
    {}
    Value(Date v)
        : storage_(v)
    {}
    Value(Time v)
        : storage_(v)
    {}
    Value(Blob v)
        : storage_(std::move(v))
    {}
    Value(Inet v)
        : storage_(std::move(v))
    {}

    Value(List v);
    Value(Map v);
    Value(Set v);

    bool IsNull() const noexcept { return std::holds_alternative<std::monostate>(storage_); }

    template <typename T>
    bool Is() const noexcept;

    template <typename T>
    const T& Get() const;

    template <typename T>
    std::optional<T> TryGet() const;

    const Storage& Raw() const noexcept { return storage_; }
    Storage& Raw() noexcept { return storage_; }

    static Value Null() noexcept { return Value{}; }

private:
    Storage storage_{};
};

struct List {
    std::vector<Value> items;
};

struct Map {
    std::vector<std::pair<Value, Value>> entries;
};

struct Set {
    std::vector<Value> items;
};

inline Value::Value(List v)
    : storage_(std::make_shared<List>(std::move(v)))
{}
inline Value::Value(Map v)
    : storage_(std::make_shared<Map>(std::move(v)))
{}
inline Value::Value(Set v)
    : storage_(std::make_shared<Set>(std::move(v)))
{}

namespace impl::value {

template <typename T>
struct AlternativeOf {
    using type = T;
};
template <>
struct AlternativeOf<List> {
    using type = std::shared_ptr<List>;
};
template <>
struct AlternativeOf<Map> {
    using type = std::shared_ptr<Map>;
};
template <>
struct AlternativeOf<Set> {
    using type = std::shared_ptr<Set>;
};

template <typename T>
using AlternativeOfT = typename AlternativeOf<T>::type;

}  // namespace impl::value

template <typename T>
bool Value::Is() const noexcept {
    return std::holds_alternative<impl::value::AlternativeOfT<T>>(storage_);
}

template <typename T>
const T& Value::Get() const {
    if constexpr (std::is_same_v<T, List> || std::is_same_v<T, Map> || std::is_same_v<T, Set>) {
        return *std::get<impl::value::AlternativeOfT<T>>(storage_);
    } else {
        return std::get<T>(storage_);
    }
}

template <typename T>
std::optional<T> Value::TryGet() const {
    if (!Is<T>()) {
        return std::nullopt;
    }
    return Get<T>();
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END
