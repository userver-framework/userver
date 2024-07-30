#include "say_hello.hpp"

#include <userver/utest/utest.hpp>

UTEST(SayHelloTo, Basic) {
  EXPECT_EQ(samples::hello::SayHelloTo({"Developer"}).text,
            "Hello, Developer!\n");
  EXPECT_EQ(samples::hello::SayHelloTo({}).text, "Hello, noname!\n");
}
