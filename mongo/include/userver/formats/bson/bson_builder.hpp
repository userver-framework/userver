#pragma once

/// @file userver/formats/bson/bson_builder.hpp
/// @brief Internal helpers for inline document build

#include <chrono>
#include <cstddef>
#include <string_view>
#include <type_traits>

#include <userver/compiler/select.hpp>
#include <userver/formats/bson/types.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson::impl {

class MutableBson;

class BsonBuilder {
public:
    BsonBuilder();
    explicit BsonBuilder(const ValueImpl&);
    ~BsonBuilder();

    BsonBuilder(const BsonBuilder&);
    BsonBuilder(BsonBuilder&&) noexcept;
    BsonBuilder& operator=(const BsonBuilder&);
    BsonBuilder& operator=(BsonBuilder&&) noexcept;

    BsonBuilder& Append(std::string_view key, std::nullptr_t);
    BsonBuilder& Append(std::string_view key, bool);
    BsonBuilder& Append(std::string_view key, int);
    BsonBuilder& Append(std::string_view key, unsigned int);
    BsonBuilder& Append(std::string_view key, long);
    BsonBuilder& Append(std::string_view key, unsigned long);
    BsonBuilder& Append(std::string_view key, long long);
    BsonBuilder& Append(std::string_view key, unsigned long long);
    BsonBuilder& Append(std::string_view key, double);
    BsonBuilder& Append(std::string_view key, const char*);
    BsonBuilder& Append(std::string_view key, std::string_view);

    template <typename Duration>
    BsonBuilder& Append(std::string_view key, std::chrono::time_point<std::chrono::system_clock, Duration> value) {
        return Append(key, std::chrono::time_point_cast<std::chrono::milliseconds>(value));
    }
    BsonBuilder&
    Append(std::string_view key, std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>);

    BsonBuilder& Append(std::string_view key, const Oid&);
    BsonBuilder& Append(std::string_view key, const Binary&);
    BsonBuilder& Append(std::string_view key, const Decimal128&);
    BsonBuilder& Append(std::string_view key, MinKey);
    BsonBuilder& Append(std::string_view key, MaxKey);
    BsonBuilder& Append(std::string_view key, const Timestamp&);

    BsonBuilder& Append(std::string_view key, const Value&);

    BsonBuilder& Append(std::string_view key, const bson_t*);

    template <typename T>
    std::enable_if_t<
        !std::is_integral_v<T> &&                           //
            !std::is_convertible_v<T, std::string_view> &&  //
            !std::is_convertible_v<T, Value>,               // Excludes Document that inherits Value
        BsonBuilder&>
    Append(std::string_view key, const T& val) {
        return Append(key, ValueBuilder(val).ExtractValue());
    }

    const bson_t* Get() const;
    bson_t* Get();

    BsonHolder Extract();

private:
    void AppendInto(bson_t*, std::string_view key, const ValueImpl&);

    static constexpr std::size_t kSize = compiler::SelectSize()  //
                                             .For64Bit(8)
                                             .For32Bit(4);
    static constexpr std::size_t kAlignment = alignof(void*);
    utils::FastPimpl<MutableBson, kSize, kAlignment, utils::kStrictMatch> bson_;
};

}  // namespace formats::bson::impl

USERVER_NAMESPACE_END
