#include <type_traits>

#include <gtest/gtest.h>

#include <maps/basic.pb.h>
#include <maps/basic.structs.usrv.pb.hpp>

#include <userver/proto-structs/convert.hpp>

USERVER_NAMESPACE_BEGIN

TEST(MapsBasic, FundamentalTypes) {
    using Struct = maps::structs::Basic;
    static_assert(std::is_same_v<decltype(Struct::string_int), proto_structs::HashMap<std::string, std::int64_t>>);
    static_assert(std::is_same_v<decltype(Struct::string_string), proto_structs::HashMap<std::string, std::string>>);
    static_assert(std::is_same_v<decltype(Struct::int_int), proto_structs::HashMap<std::int64_t, std::int64_t>>);
    static_assert(std::is_same_v<decltype(Struct::int_string), proto_structs::HashMap<std::int64_t, std::string>>);

    Struct s{};
    s.int_int = {{1, 2}, {3, 4}};
    s.int_string = {{1, "2"}, {3, "4"}};
    s.string_int = {{"1", 2}, {"3", 4}};
    s.string_string = {{"1", "2"}, {"3", "4"}};

    Struct::ProtobufMessage vanilla{};
    proto_structs::StructToMessage(std::move(s), vanilla);
    {
        auto map = vanilla.int_int();
        ASSERT_EQ(map.size(), 2u);
        ASSERT_EQ(map.at(1), 2);
        ASSERT_EQ(map.at(3), 4);
    }
    {
        auto map = vanilla.int_string();
        ASSERT_EQ(map.size(), 2u);
        ASSERT_EQ(map.at(1), "2");
        ASSERT_EQ(map.at(3), "4");
    }
    {
        auto map = vanilla.string_int();
        ASSERT_EQ(map.size(), 2u);
        ASSERT_EQ(map.at("1"), 2);
        ASSERT_EQ(map.at("3"), 4);
    }
    {
        auto map = vanilla.string_string();
        ASSERT_EQ(map.size(), 2u);
        ASSERT_EQ(map.at("1"), "2");
        ASSERT_EQ(map.at("3"), "4");
    }
}

USERVER_NAMESPACE_END
