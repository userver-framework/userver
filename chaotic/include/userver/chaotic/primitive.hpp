#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/formats/json/string_builder_fwd.hpp>
#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename RawType, typename... Validators>
struct Primitive final {
    const RawType& value;
};

template <typename Value, typename RawType, typename... Validators>
RawType Parse(const Value& value, formats::parse::To<Primitive<RawType, Validators...>>) {
    auto result = value.template As<RawType>();
    chaotic::Validate<Validators...>(result, value);
    return result;
}

template <typename Value, typename RawType, typename... Validators>
Value Serialize(const Primitive<RawType, Validators...>& ps, formats::serialize::To<Value>) {
    return typename Value::Builder{ps.value}.ExtractValue();
}

template <typename RawType, typename... Validators>
void WriteToStream(
    const Primitive<RawType, Validators...>& ps,
    formats::json::StringBuilder& sw,
    bool hide_brackets,
    std::string_view hide_field_name = {}
) {
    WriteToStream(ps.value, sw, hide_brackets, hide_field_name);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
