#include <gtest/gtest.h>

#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

// The same name as in any_storage_test.cpp
struct MyStorage {};

const utils::AnyStorageDataTag<MyStorage, int> kInt2;

TEST(AnyStorage, Simple2) {
  utils::AnyStorage<MyStorage> storage;

  EXPECT_EQ(storage.GetOptional(kInt2), nullptr);

  storage.Emplace(kInt2, 1);

  EXPECT_EQ(*storage.GetOptional(kInt2), 1);

  EXPECT_EQ(storage.Get(kInt2), 1);

  storage.Set(kInt2, 2);

  EXPECT_EQ(storage.Get(kInt2), 2);
}

USERVER_NAMESPACE_END
