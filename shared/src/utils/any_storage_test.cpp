#include <gtest/gtest.h>

#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

/// [AnyStorage]
struct MyStorage {};

const utils::AnyStorageDataTag<MyStorage, int> kInt;
const utils::AnyStorageDataTag<MyStorage, std::string> kStr;

TEST(AnyStorage, Simple) {
  utils::AnyStorage<MyStorage> storage;

  EXPECT_EQ(storage.GetOptional(kInt), nullptr);

  storage.Emplace(kInt, 1);
  storage.Emplace(kStr, std::string{"1"});

  EXPECT_EQ(*storage.GetOptional(kInt), 1);

  EXPECT_EQ(storage.Get(kInt), 1);
  EXPECT_EQ(storage.Get(kStr), "1");

  storage.Set(kInt, 2);
  storage.Set(kStr, std::string{"2"});

  EXPECT_EQ(storage.Get(kInt), 2);
  EXPECT_EQ(storage.Get(kStr), "2");

  storage.Emplace(kStr, std::string{"3"});
  storage.Emplace(kStr, std::string{"4"});
  EXPECT_EQ(storage.Get(kStr), "4");
}
/// [AnyStorage]

USERVER_NAMESPACE_END
