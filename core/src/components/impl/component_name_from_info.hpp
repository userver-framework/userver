#pragma once

#include <string_view>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

// Reference to a name from ComponentInfo or a temporary object to search in
// container of ComponentNameFromInfo
class ComponentNameFromInfo : public utils::StrongTypedef<ComponentNameFromInfo, std::string_view> {
public:
    using utils::StrongTypedef<ComponentNameFromInfo, std::string_view>::StrongTypedef;

    std::string_view StringViewName() const noexcept { return GetUnderlying(); }
};

}  // namespace components::impl

USERVER_NAMESPACE_END

template <>
struct std::hash<USERVER_NAMESPACE::components::impl::ComponentNameFromInfo> {
    std::size_t operator()(const USERVER_NAMESPACE::components::impl::ComponentNameFromInfo& v) const noexcept {
        return std::hash<std::string_view>{}(v.StringViewName());
    }
};

// fmt::format support
template <>
struct fmt::formatter<USERVER_NAMESPACE::components::impl::ComponentNameFromInfo, char>
    : fmt::formatter<std::string_view, char> {
    template <typename FormatContext>
    auto format(const USERVER_NAMESPACE::components::impl::ComponentNameFromInfo& v, FormatContext& ctx)
        USERVER_FMT_CONST {
        return fmt::formatter<std::string_view, char>{}.format(v.StringViewName(), ctx);
    }
};
