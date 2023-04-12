#include "hello.hpp"

#include <userver/utest/utest.hpp>

UTEST(SayHelloTo, Basic) {
  EXPECT_EQ(service_template::SayHelloTo("Developer"), "Hello, Developer!\n");
  EXPECT_EQ(service_template::SayHelloTo({}), "Hello, unknown user!\n");
}
