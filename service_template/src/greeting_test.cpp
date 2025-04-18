#include <greeting.hpp>

#include <userver/utest/utest.hpp>

using service_template::UserType;

UTEST(SayHelloTo, Basic) {
    EXPECT_EQ(service_template::SayHelloTo("Developer", UserType::kFirstTime), "Hello, Developer!\n");
    EXPECT_EQ(service_template::SayHelloTo({}, UserType::kFirstTime), "Hello, unknown user!\n");

    EXPECT_EQ(service_template::SayHelloTo("Developer", UserType::kKnown), "Hi again, Developer!\n");
    EXPECT_EQ(service_template::SayHelloTo({}, UserType::kKnown), "Hi again, unknown user!\n");
}
