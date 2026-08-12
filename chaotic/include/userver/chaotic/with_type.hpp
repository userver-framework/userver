#pragma once

#include <userver/chaotic/convert.hpp>
#include <userver/chaotic/convert/to.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/string_builder_fwd.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename RawType, typename UserType>
struct WithType final {
    const UserType& value;
};

template <typename Value, typename RawType, typename UserType>
UserType Parse(const Value& value, formats::parse::To<WithType<RawType, UserType>>) {
    auto result = value.template As<RawType>();
    try {
        return chaotic::ConvertTo<UserType>(std::move(result));
    } catch (const std::exception& e) {
        chaotic::ThrowForValue(e.what(), value);
    }
}

template <typename Value, typename RawType, typename UserType>
Value Serialize(const WithType<RawType, UserType>& ps, formats::serialize::To<Value>) {
    using RawTypeDecayed = std::decay_t<decltype(RawType::value)>;
    return typename Value::Builder{RawType{chaotic::ConvertTo<RawTypeDecayed>(ps.value)}}.ExtractValue();
}

template <typename RawType, typename UserType>
void WriteToStream(const WithType<RawType, UserType>& ps, formats::json::StringBuilder& sw) {
    using RawTypeDecayed = std::decay_t<decltype(RawType::value)>;
    WriteToStream(RawType{chaotic::ConvertTo<RawTypeDecayed>(ps.value)}, sw);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
