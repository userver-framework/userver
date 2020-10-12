#include <gtest/gtest.h>

#include <utils/regex.hpp>

TEST(Regex, Ctors) {
  utils::regex r1;
  utils::regex r2("regex*test");
  utils::regex r3(std::move(r2));
  utils::regex r4(r3);
  utils::regex r5;
  r5 = std::move(r4);
  utils::regex r6;
  r6 = r5;
}
