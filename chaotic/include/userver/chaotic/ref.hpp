#pragma once

#include <userver/formats/common/meta.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/box.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename T>
struct Ref {
    const utils::Box<formats::common::ParseType<formats::json::Value, T>>& value;
};

template <typename Value, typename T>
utils::Box<formats::common::ParseType<Value, T>> Parse(const Value& value, formats::parse::To<Ref<T>>) {
    auto result = value.template As<T>();
    return result;
}

template <typename Value, typename T>
Value Serialize(const Ref<T>& ps, formats::serialize::To<Value>) {
    return typename Value::Builder{T{*ps.value}}.ExtractValue();
}

}  // namespace chaotic

USERVER_NAMESPACE_END
