#include <gtest/gtest.h>

#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

/// [AnyStorage]
struct MyStorage {};

const utils::AnyStorageDataTag<MyStorage, char> kChar;
const utils::AnyStorageDataTag<MyStorage, int> kInt;
const utils::AnyStorageDataTag<MyStorage, std::string> kStr;
const utils::AnyStorageDataTag<MyStorage, std::string> kStr2;

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

TEST(AnyStorage, Alignment) {
  utils::AnyStorage<MyStorage> storage;

  storage.Emplace(kChar, '1');
  storage.Emplace(kInt, 1);
  storage.Emplace(kStr, std::string{"1"});
  storage.Emplace(kStr2, std::string{"1"});

  EXPECT_EQ(reinterpret_cast<uintptr_t>(&storage.Get(kInt)) % alignof(int), 0);
  EXPECT_EQ(
      reinterpret_cast<uintptr_t>(&storage.Get(kStr)) % alignof(std::string),
      0);
  EXPECT_EQ(&storage.Get(kStr) + 1, &storage.Get(kStr2));
}

USERVER_NAMESPACE_END
