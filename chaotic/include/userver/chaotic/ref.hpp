#pragma once

#include <userver/formats/common/meta.hpp>
#include <userver/formats/json/string_builder_fwd.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/may_parse.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/box.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename T>
struct Ref {
    const utils::Box<formats::common::ParseType<formats::json::Value, T>>& value;
};

template <typename Value, typename T>
utils::Box<formats::common::ParseType<Value, T>> Parse(const Value& value, formats::parse::To<Ref<T>>) {
    return utils::Box<formats::common::ParseType<Value, T>>::MakeWithFactory([&value] { return value.template As<T>(); }
    );
}

/// @brief Returns `false` if parsing `T` is certain to fail, see @ref formats::parse::MayParseAs.
template <typename Value, typename T>
bool MayParseAs(const Value& value, formats::parse::To<Ref<T>>) {
    return formats::parse::MayParseAs<T>(value);
}

template <typename Value, typename T>
Value Serialize(const Ref<T>& ps, formats::serialize::To<Value>) {
    return typename Value::Builder{T{*ps.value}}.ExtractValue();
}

template <typename T>
void WriteToStream(const Ref<T>& ps, formats::json::StringBuilder& sw) {
    WriteToStream(T{*ps.value}, sw);
}

template <typename T>
void WriteToStream(
    const Ref<T>& ps,
    formats::json::StringBuilder& sw,
    bool hide_brackets,
    std::string_view hide_field_name
) {
    WriteToStream(T{*ps.value}, sw, hide_brackets, hide_field_name);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
