#include <gtest/gtest.h>

#include <simple/file1.structs.usrv.pb.hpp>

TEST(SingleFile, SimpleStruct) {
    namespace ss = simple::structs;
    [[maybe_unused]] ss::SimpleStruct message;
    [[maybe_unused]] ss::SimpleStruct::NestedStruct nested;
    [[maybe_unused]] ss::SimpleStruct::NestedStruct::NestedStruct2 nested2;
}

TEST(SingleFile, SecondStruct) { [[maybe_unused]] simple::structs::SecondStruct message; }
