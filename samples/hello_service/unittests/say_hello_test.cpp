/// [Unit test]
#include "say_hello.hpp"

#include <userver/utest/utest.hpp>

UTEST(SayHelloTo, Basic) {
    EXPECT_EQ(samples::hello::SayHelloTo("Developer"), "Hello, Developer!\n");
    EXPECT_EQ(samples::hello::SayHelloTo({}), "Hello, unknown user!\n");
}
/// [Unit test]
