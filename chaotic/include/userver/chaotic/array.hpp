#pragma once

#include <vector>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/meta.hpp>

#include <userver/chaotic/validators.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename ItemType, typename UserType, typename... Validators>
struct Array final {
    const UserType& value;
};

template <typename Value, typename ItemType, typename UserType, typename... Validators>
UserType Parse(const Value& value, formats::parse::To<Array<ItemType, UserType, Validators...>>) {
    UserType arr;
    auto inserter = std::inserter(arr, arr.end());
    if constexpr (meta::kIsReservable<UserType>) {
        arr.reserve(value.GetSize());
    }
    for (const auto& item : value) {
        *inserter = item.template As<ItemType>();
        ++inserter;
    }

    chaotic::Validate<Validators...>(arr, value);

    return arr;
}

template <typename Value, typename ItemType, typename... Validators>
std::vector<formats::common::ParseType<Value, ItemType>>
Parse(const Value& value, formats::parse::To<Array<ItemType, std::vector<formats::common::ParseType<Value, ItemType>>, Validators...>>) {
    std::vector<formats::common::ParseType<Value, ItemType>> arr;
    arr.reserve(value.GetSize());
    for (const auto& item : value) {
        arr.emplace_back(item.template As<ItemType>());
    }

    chaotic::Validate<Validators...>(arr, value);

    return arr;
}

template <typename Value, typename ItemType, typename UserType, typename... Validators>
Value Serialize(const Array<ItemType, UserType, Validators...>& ps, formats::serialize::To<Value>) {
    typename Value::Builder vb{formats::common::Type::kArray};
    for (const auto& item : ps.value) {
        vb.PushBack(ItemType{item});
    }
    return vb.ExtractValue();
}

}  // namespace chaotic

USERVER_NAMESPACE_END
