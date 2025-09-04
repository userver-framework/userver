#pragma once

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl::traits {

template <template <typename...> typename Template, typename... TArgs>
void InheritsFromInstantiationImpl(const volatile Template<TArgs...>&) {}

template <template <typename...> typename Template, typename T>
inline constexpr bool InheritsFromInstantiation =
    !std::is_reference_v<T> && requires(T derived) { traits::InheritsFromInstantiationImpl<Template>(derived); };

}  // namespace proto_structs::impl::traits

USERVER_NAMESPACE_END
