#include <type_traits>

#include <gtest/gtest.h>

#include <enums/names.structs.usrv.pb.hpp>

TEST(EnumNames, Unprefixed) {
    using Enum = enums::structs::Unprefixed;
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<decltype(Enum::FOO_VAR), Enum>);
    static_assert(std::is_same_v<decltype(Enum::BAR_VAR), Enum>);
}

TEST(EnumNames, AllowedCuts) {
    using Enum = enums::structs::AllowedCuts;
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<decltype(Enum::FOO_VAR), Enum>);
    static_assert(std::is_same_v<decltype(Enum::DIGITS1), Enum>);
}

TEST(EnumNames, DisallowedCuts) {
    using Enum = enums::structs::DisallowedCuts;
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<decltype(Enum::DisallowedCuts_UNKNOWN), Enum>);
    static_assert(std::is_same_v<decltype(Enum::DISALLOWED_CUTS), Enum>);
    static_assert(std::is_same_v<decltype(Enum::DISALLOWED_CUTS1), Enum>);
    static_assert(std::is_same_v<decltype(Enum::DISALLOWED_CUTS_2), Enum>);
    static_assert(std::is_same_v<decltype(Enum::DisallowedCutsCamel), Enum>);
}

TEST(EnumNames, NestedTrick) {
    using Enum = enums::structs::NestedTrickEnum;
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<decltype(Enum::FOO_VAR), Enum>);
}
