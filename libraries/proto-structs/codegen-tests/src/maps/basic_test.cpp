#include <type_traits>

#include <gtest/gtest.h>

#include <maps/basic.structs.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

TEST(MapsBasic, FundamentalTypes) {
    using Struct = maps::structs::Basic;
    static_assert(std::is_same_v<decltype(Struct::string_int), proto_structs::HashMap<std::string, std::int64_t>>);
    static_assert(std::is_same_v<decltype(Struct::string_string), proto_structs::HashMap<std::string, std::string>>);
    static_assert(std::is_same_v<decltype(Struct::int_int), proto_structs::HashMap<std::int64_t, std::int64_t>>);
    static_assert(std::is_same_v<decltype(Struct::int_string), proto_structs::HashMap<std::int64_t, std::string>>);
}

USERVER_NAMESPACE_END
