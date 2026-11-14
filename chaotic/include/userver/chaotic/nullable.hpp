#pragma once

#include <optional>
#include <string_view>

#include <userver/chaotic/object.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/parse/may_parse.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

/// @brief Descriptor of a `nullable` value outside of a structure field, e.g. of `additionalProperties`:
/// `std::optional` of the value of `Descriptor`, JSON `null` is `std::nullopt`.
template <typename Descriptor>
struct Nullable final {
    const std::optional<TypeOfDescriptor<Descriptor>>& value;
};

template <typename Value, typename Descriptor>
std::optional<TypeOfDescriptor<Descriptor>> Parse(const Value& value, formats::parse::To<Nullable<Descriptor>>) {
    if (value.IsNull()) {
        return std::nullopt;
    }
    return value.template As<Descriptor>();
}

/// @brief Returns `false` if parsing `Descriptor` is certain to fail, see @ref formats::parse::MayParseAs.
template <typename Value, typename Descriptor>
bool MayParseAs(const Value& value, formats::parse::To<Nullable<Descriptor>>) {
    return value.IsNull() || formats::parse::MayParseAs<Descriptor>(value);
}

template <typename Value, typename Descriptor>
Value Serialize(const Nullable<Descriptor>& ps, formats::serialize::To<Value>) {
    if (!ps.value) {
        return typename Value::Builder{formats::common::Type::kNull}.ExtractValue();
    }
    return typename Value::Builder{Descriptor{*ps.value}}.ExtractValue();
}

template <typename Descriptor>
void WriteToStream(const Nullable<Descriptor>& ps, formats::json::StringBuilder& sw) {
    if (!ps.value) {
        sw.WriteNull();
        return;
    }
    WriteToStream(Descriptor{*ps.value}, sw);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
