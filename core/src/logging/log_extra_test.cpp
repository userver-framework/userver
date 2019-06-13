#include <utest/utest.hpp>

#include <logging/log_extra.hpp>

TEST(LogExtra, Types) {
  logging::LogExtra le{
      {"unsigned int", 1U},          //
      {"int", 1},                    //
      {"long", 1L},                  //
      {"long long", 1LL},            //
      {"unsinged long", 1UL},        //
      {"unsinged long long", 1ULL},  //
      {"size_t", sizeof(1)},         //
  };
}
