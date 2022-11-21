#include <gtest/gtest.h>

#include <userver/cache/impl/doorkeeper.hpp>
#include <userver/cache/impl/hash.hpp>

USERVER_NAMESPACE_BEGIN

using Jenkins = cache::impl::tools::Jenkins<int>;
using Doorkeeper = cache::impl::Doorkeeper<int, Jenkins>;

TEST(Doorkeeper, Put) {
  Doorkeeper doorkeeper(512, Jenkins{});
  doorkeeper.Put(0);
  doorkeeper.Contains(0);
}

TEST(Doorkeeper, Contains) {
  Doorkeeper doorkeeper(512, Jenkins{});

  for (int i = 0; i < 512; i += 10) doorkeeper.Put(i);

  for (int i = 0; i < 512; i += 10) EXPECT_TRUE(doorkeeper.Contains(i));
}

TEST(Doorkeeper, Basic) {
  Doorkeeper doorkeeper(512, Jenkins{});

  for (int i = 0; i < 512; i += 10) doorkeeper.Put(i);

  EXPECT_FALSE(doorkeeper.Contains(1) && doorkeeper.Contains(101) &&
               doorkeeper.Contains(201));
}

USERVER_NAMESPACE_END