#pragma once

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl::traits {

template <template <typename...> typename Template, typename T>
struct InheritsFromInstantiation {
private:
    template <typename... TArgs>
    static constexpr std::true_type Test(Template<TArgs...>&&) {
        return {};
    }

    static constexpr std::false_type Test(...) { return {}; }

public:
    static constexpr inline bool value =
        std::is_same_v<decltype(Test(std::declval<std::remove_cv_t<T>>())), std::true_type>;
};

template <template <typename...> typename Template, typename T>
inline constexpr bool kInheritsFromInstantiation = InheritsFromInstantiation<Template, T>::value;

}  // namespace proto_structs::impl::traits

USERVER_NAMESPACE_END
