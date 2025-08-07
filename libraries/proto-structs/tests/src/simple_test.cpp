#include <gtest/gtest.h>

#include <simple/file1.structs.usrv.pb.hpp>

TEST(SingleFile, SimpleStruct) {
    namespace ss = simple::structs;
    [[maybe_unused]] ss::SimpleStruct message;
    [[maybe_unused]] ss::SimpleStruct::NestedStruct nested;
    [[maybe_unused]] ss::SimpleStruct::NestedStruct::NestedStruct2 nested2;
    [[maybe_unused]] ss::SimpleStruct::InnerEnum2 inner_enum2{};
    [[maybe_unused]] ss::SimpleStruct::NestedStruct::NestedStruct2::InnerEnum1 inner_enum1{};
}

TEST(SingleFile, SecondStruct) { [[maybe_unused]] simple::structs::SecondStruct message; }

TEST(SingleFile, GlobalEnum) { [[maybe_unused]] simple::structs::GlobalEnum message{}; }
