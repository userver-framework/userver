#include <gtest/gtest.h>

#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

/// [AnyStorage]
struct MyStorage {};

const utils::AnyStorageDataTag<MyStorage, char> kChar;
const utils::AnyStorageDataTag<MyStorage, int> kInt;
const utils::AnyStorageDataTag<MyStorage, std::string> kStr;
const utils::AnyStorageDataTag<MyStorage, std::string> kStr2;

struct BoolStorage {};

const utils::AnyStorageDataTag<BoolStorage, bool> kBool;

TEST(AnyStorage, Move) {
    utils::AnyStorage<MyStorage> storage;
    auto tmp = std::move(storage);
    SUCCEED();
}

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

    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&storage.Get(kInt)) % alignof(int), 0);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&storage.Get(kStr)) % alignof(std::string), 0);
    EXPECT_EQ(&storage.Get(kStr) + 1, &storage.Get(kStr2));
}

TEST(AnyStorage, BoolData) {
    utils::AnyStorage<BoolStorage> storage;

    storage.Emplace(kBool, true);

    EXPECT_EQ(storage.Get(kBool), true);
}

namespace {

struct LifetimeStorage {};

struct TrackedData final {
    explicit TrackedData(int& destructions, bool should_throw = false)
        : destructions(destructions)
    {
        if (should_throw) {
            throw std::runtime_error("Construction failed");
        }
    }

    TrackedData(const TrackedData&) = delete;
    TrackedData(TrackedData&&) = delete;

    ~TrackedData() { ++destructions; }

    int& destructions;
};

const utils::AnyStorageDataTag<LifetimeStorage, TrackedData> kTracked;

}  // namespace

TEST(AnyStorage, EraseAndReuse) {
    int destructions = 0;
    {
        utils::AnyStorage<LifetimeStorage> storage;
        storage.Erase(kTracked);
        EXPECT_EQ(destructions, 0);

        storage.Emplace(kTracked, destructions);
        storage.Erase(kTracked);
        EXPECT_EQ(destructions, 1);
        EXPECT_EQ(storage.GetOptional(kTracked), nullptr);
        EXPECT_THROW(storage.Get(kTracked), std::runtime_error);

        storage.Erase(kTracked);
        EXPECT_EQ(destructions, 1);
        storage.Emplace(kTracked, destructions);
    }
    EXPECT_EQ(destructions, 2);
}

TEST(AnyStorage, ThrowingReplacementLeavesEmptySlot) {
    int destructions = 0;
    {
        utils::AnyStorage<LifetimeStorage> storage;
        storage.Emplace(kTracked, destructions);
        EXPECT_THROW(storage.Emplace(kTracked, destructions, true), std::runtime_error);
        EXPECT_EQ(destructions, 1);
        EXPECT_EQ(storage.GetOptional(kTracked), nullptr);
        EXPECT_THROW(storage.Get(kTracked), std::runtime_error);
    }
    EXPECT_EQ(destructions, 1);
}

TEST(AnyStorage, ReuseAfterThrowingConstruction) {
    int destructions = 0;
    {
        utils::AnyStorage<LifetimeStorage> storage;
        EXPECT_THROW(storage.Emplace(kTracked, destructions, true), std::runtime_error);
        EXPECT_EQ(storage.GetOptional(kTracked), nullptr);
        storage.Emplace(kTracked, destructions);
        EXPECT_THROW(storage.Emplace(kTracked, destructions, true), std::runtime_error);
        storage.Emplace(kTracked, destructions);
        EXPECT_EQ(destructions, 1);
    }
    EXPECT_EQ(destructions, 2);
}

USERVER_NAMESPACE_END
