#include <type_traits>

#include <gtest/gtest.h>

#include <enums/names.structs.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

TEST(EnumNames, Unprefixed) {
    using Enum = enums::structs::Unprefixed;
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<decltype(Enum::kFooVar), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kBarVar), Enum>);
}

TEST(EnumNames, AllowedCuts) {
    using Enum = enums::structs::AllowedCuts;
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<decltype(Enum::kFooVar), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kDigits1), Enum>);
}

TEST(EnumNames, DisallowedCuts) {
    using Enum = enums::structs::DisallowedCuts;
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<decltype(Enum::kDisallowedCutsUnknown), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kDisallowedCuts), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kDisallowedCuts1), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kDisallowedCuts2), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kDisallowedCutsCamel), Enum>);
}

TEST(EnumNames, NestedTrick) {
    using Enum = enums::structs::NestedTrickEnum;
    static_assert(std::is_enum_v<Enum>);
    static_assert(std::is_same_v<decltype(Enum::kFooVar), Enum>);
}

TEST(EnumNames, WithoutPrefix) {
    using Enum = enums::structs::WithoutPrefix;
    static_assert(std::is_same_v<decltype(Enum::kBarVar), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kBarFoo), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kBarFoo1), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kBarFooQux), Enum>);
    static_assert(std::is_same_v<decltype(Enum::kBarFooQuX1), Enum>);
}

USERVER_NAMESPACE_END
