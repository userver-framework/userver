#include <userver/utils/slot_map.hpp>

#include <deque>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/container/small_vector.hpp>

#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename Value>
using DequeStorage = std::deque<Value>;

template <typename Value>
using VectorStorage = std::vector<Value>;

template <typename Value>
using SmallVectorStorage = boost::container::small_vector<Value, 4>;

template <template <typename> typename ContainerParam>
struct SlotMapStorage final {
    template <typename Value>
    using Container = ContainerParam<Value>;
};

template <typename Storage, typename Value>
using SlotMapFor = utils::SlotMap<Value, Storage::template Container>;

using SlotMapStorageTypes =
    ::testing::Types<SlotMapStorage<DequeStorage>, SlotMapStorage<VectorStorage>, SlotMapStorage<SmallVectorStorage>>;

template <typename Storage>
class SlotMapByUnderlyingContainer : public ::testing::Test {};

TYPED_TEST_SUITE(SlotMapByUnderlyingContainer, SlotMapStorageTypes);

}  // namespace

TYPED_TEST(SlotMapByUnderlyingContainer, DefaultConstructedIsEmpty) {
    SlotMapFor<TypeParam, int> m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
}

TYPED_TEST(SlotMapByUnderlyingContainer, EmplaceReturnsIndexAndReference) {
    SlotMapFor<TypeParam, int> m;
    auto result = m.emplace(42);
    EXPECT_EQ(result.index, 0);
    EXPECT_EQ(result.element, 42);
    EXPECT_EQ(m.size(), 1);
    EXPECT_FALSE(m.empty());
}

TYPED_TEST(SlotMapByUnderlyingContainer, MultipleEmplaceIncreasesSize) {
    SlotMapFor<TypeParam, int> m;
    const auto idx0 = m.insert(1).index;
    const auto idx1 = m.insert(2).index;
    const auto idx2 = m.insert(3).index;
    EXPECT_EQ(idx0, 0);
    EXPECT_EQ(idx1, 1);
    EXPECT_EQ(idx2, 2);
    EXPECT_EQ(m.size(), 3);
}

TYPED_TEST(SlotMapByUnderlyingContainer, OperatorBracketAccess) {
    SlotMapFor<TypeParam, std::string> m;
    auto result = m.insert("hello");
    const auto idx = result.index;
    EXPECT_EQ(m[idx], "hello");
    m[idx] = "world";
    EXPECT_EQ(m[idx], "world");
    EXPECT_EQ(result.element, "world");
}

TYPED_TEST(SlotMapByUnderlyingContainer, ConstOperatorBracketAccess) {
    SlotMapFor<TypeParam, int> m;
    const auto idx = m.insert(99).index;
    const auto& cm = m;
    EXPECT_EQ(cm[idx], 99);
}

TYPED_TEST(SlotMapByUnderlyingContainer, InsertCopy) {
    SlotMapFor<TypeParam, std::string> m;
    const std::string s = "copy";
    auto result = m.insert(s);
    EXPECT_EQ(result.element, "copy");
    EXPECT_EQ(m[result.index], "copy");
}

TYPED_TEST(SlotMapByUnderlyingContainer, InsertMove) {
    SlotMapFor<TypeParam, std::vector<int>> m;
    std::vector<int> v = {1, 2, 3};
    auto result = m.insert(std::move(v));
    // The source must have been moved, not copied.
    EXPECT_TRUE(v.empty());  // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(result.element, (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(m[result.index], (std::vector<int>{1, 2, 3}));
}

TYPED_TEST(SlotMapByUnderlyingContainer, InsertRange) {
    std::vector<int> v = {10, 20, 30};
    SlotMapFor<TypeParam, int> m;
    m.insert_range(v);
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[0], 10);
    EXPECT_EQ(m[1], 20);
    EXPECT_EQ(m[2], 30);
}

TYPED_TEST(SlotMapByUnderlyingContainer, IteratorConstructor) {
    std::vector<int> v = {5, 6, 7};
    SlotMapFor<TypeParam, int> m(v.begin(), v.end());
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[0], 5);
    EXPECT_EQ(m[1], 6);
    EXPECT_EQ(m[2], 7);
}

TYPED_TEST(SlotMapByUnderlyingContainer, EraseReducesSize) {
    SlotMapFor<TypeParam, int> m;
    const auto idx0 = m.insert(1).index;
    const auto idx1 = m.insert(2).index;
    const auto idx2 = m.insert(3).index;

    EXPECT_EQ(m.erase(idx1), 1);
    EXPECT_EQ(m.size(), 2);
    EXPECT_EQ(m[idx0], 1);
    EXPECT_EQ(m[idx2], 3);
}

TYPED_TEST(SlotMapByUnderlyingContainer, EraseAlreadyErasedIsNoop) {
    SlotMapFor<TypeParam, int> m;
    const auto idx0 = m.insert(1).index;
    m.insert(2);

    EXPECT_EQ(m.erase(idx0), 1);
    EXPECT_EQ(m.size(), 1);

    // Erasing the same slot again must be a no-op and report 0 erased elements.
    EXPECT_EQ(m.erase(idx0), 0);
    EXPECT_EQ(m.size(), 1);

    // The remaining live element must still be accessible.
    EXPECT_EQ(m[1], 2);
}

TYPED_TEST(SlotMapByUnderlyingContainer, EraseAndReuseSlot) {
    SlotMapFor<TypeParam, int> m;
    const auto idx0 = m.insert(10).index;
    m.insert(20);

    EXPECT_EQ(m.erase(idx0), 1);
    EXPECT_EQ(m.size(), 1);

    // The freed slot (idx0) should be reused.
    const auto idx2 = m.insert(30).index;
    EXPECT_EQ(idx2, idx0);
    EXPECT_EQ(m[idx2], 30);
    EXPECT_EQ(m.size(), 2);
}

TYPED_TEST(SlotMapByUnderlyingContainer, IndexesAreStableAfterErase) {
    SlotMapFor<TypeParam, int> m;
    const auto idx0 = m.insert(1).index;
    const auto idx1 = m.insert(2).index;
    const auto idx2 = m.insert(3).index;

    m.erase(idx1);

    // idx0 and idx2 must still be valid and unchanged.
    EXPECT_EQ(m[idx0], 1);
    EXPECT_EQ(m[idx2], 3);
}

TEST(SlotMap, DequeReferencesAreStableAfterEmplace) {
    utils::SlotMap<int> m;
    auto result = m.emplace(-1);
    int* ptr0 = &result.element;

    // Emplace many more elements to force deque growth.
    for (int i = 0; i < 100; ++i) {
        m.emplace(i);
    }

    // Reference/pointer to the first element must still be valid.
    EXPECT_EQ(*ptr0, -1);
    EXPECT_EQ(m[result.index], -1);
}

TYPED_TEST(SlotMapByUnderlyingContainer, AliveItemsIteratesLiveElements) {
    SlotMapFor<TypeParam, int> m;
    m.insert(1);
    const auto idx1 = m.insert(2).index;
    m.insert(3);

    m.erase(idx1);

    std::vector<int> alive;
    for (int& v : m.AliveItems()) {
        alive.push_back(v);
    }
    EXPECT_EQ(alive, (std::vector<int>{1, 3}));
}

TYPED_TEST(SlotMapByUnderlyingContainer, AliveItemsConstOverload) {
    SlotMapFor<TypeParam, int> m;
    m.insert(10);
    m.insert(20);

    const auto& cm = m;
    std::vector<int> alive;
    for (const int& v : cm.AliveItems()) {
        alive.push_back(v);
    }
    EXPECT_EQ(alive, (std::vector<int>{10, 20}));
}

TYPED_TEST(SlotMapByUnderlyingContainer, AliveItemsEmptyMap) {
    SlotMapFor<TypeParam, int> m;
    std::size_t count = 0;
    for ([[maybe_unused]] int& v : m.AliveItems()) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TYPED_TEST(SlotMapByUnderlyingContainer, AliveItemsAfterAllErased) {
    SlotMapFor<TypeParam, int> m;
    const auto idx0 = m.insert(1).index;
    const auto idx1 = m.insert(2).index;
    m.erase(idx0);
    m.erase(idx1);

    EXPECT_TRUE(m.empty());
    std::size_t count = 0;
    for ([[maybe_unused]] int& v : m.AliveItems()) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TYPED_TEST(SlotMapByUnderlyingContainer, AliveItemsMutation) {
    SlotMapFor<TypeParam, int> m;
    m.insert(1);
    m.insert(2);
    m.insert(3);

    for (int& v : m.AliveItems()) {
        v *= 10;
    }

    EXPECT_EQ(m[0], 10);
    EXPECT_EQ(m[1], 20);
    EXPECT_EQ(m[2], 30);
}

TEST(SlotMap, DequeMoveConstruction) {
    utils::SlotMap<std::unique_ptr<int>> m;
    auto r0 = m.insert(std::make_unique<int>(42));
    auto r1 = m.insert(std::make_unique<int>(43));

    utils::SlotMap<std::unique_ptr<int>> m2{std::move(m)};
    EXPECT_TRUE(m.empty());  // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(m2.size(), 2);
    // References obtained before the move still point to live elements.
    EXPECT_EQ(*r0.element, 42);
    EXPECT_EQ(*r1.element, 43);
}

TEST(SlotMap, DequeMoveAssignment) {
    utils::SlotMap<std::unique_ptr<int>> m;
    auto r0 = m.insert(std::make_unique<int>(7));

    utils::SlotMap<std::unique_ptr<int>> m2;
    m2.insert(std::make_unique<int>(99));
    m2 = std::move(m);

    EXPECT_TRUE(m.empty());  // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(m2.size(), 1);
    // Reference obtained before the move still points to the live element.
    EXPECT_EQ(*r0.element, 7);
}

TYPED_TEST(SlotMapByUnderlyingContainer, EraseAllThenRefill) {
    constexpr std::size_t kItemCount = 1000;

    SlotMapFor<TypeParam, int> m;
    // Initial fill: indices must be sequential 0..kCount-1.
    for (std::size_t i = 0; i < kItemCount; ++i) {
        EXPECT_EQ(m.insert(static_cast<int>(i)).index, i);
    }
    const std::size_t cap = m.capacity();

    // Repeat erase-all + refill 5 times; capacity must never grow.
    for (int round = 0; round < 5; ++round) {
        for (std::size_t idx = 0; idx < kItemCount; ++idx) {
            m.erase(idx);
        }
        EXPECT_TRUE(m.empty()) << "round " << round;
        EXPECT_EQ(m.capacity(), cap) << "round " << round;

        for (std::size_t i = 0; i < kItemCount; ++i) {
            EXPECT_LT(m.insert(static_cast<int>(i) * 10).index, cap) << "round " << round;
        }
        EXPECT_EQ(m.size(), kItemCount) << "round " << round;
        EXPECT_EQ(m.capacity(), cap) << "round " << round;
    }
}

TYPED_TEST(SlotMapByUnderlyingContainer, SizeAndEmptyConsistency) {
    SlotMapFor<TypeParam, int> m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);

    m.insert(1);
    EXPECT_FALSE(m.empty());
    EXPECT_EQ(m.size(), 1);

    m.insert(2);
    EXPECT_EQ(m.size(), 2);

    m.erase(0);
    EXPECT_EQ(m.size(), 1);
    EXPECT_FALSE(m.empty());

    m.erase(1);
    EXPECT_EQ(m.size(), 0);
    EXPECT_TRUE(m.empty());
}

TYPED_TEST(SlotMapByUnderlyingContainer, EmplaceThrowingConstructorDoesNotLeakFreeNode) {
    SlotMapFor<TypeParam, int> m;

    // Establish a free slot: insert then erase.
    const auto idx = m.insert(1).index;
    m.erase(idx);

    // capacity() == 1: one allocated slot, currently free.
    const std::size_t cap = m.capacity();
    EXPECT_GE(cap, 1);
    EXPECT_EQ(m.size(), 0);

    // Repeatedly emplace via a throwing factory and erase.
    // The free node must be restored on each throw so capacity never grows.
    for (int i = 0; i < 1000; ++i) {
        EXPECT_THROW(
            m.emplace(utils::LazyPrvalue{[]() -> int { throw std::runtime_error{"bang"}; }}),
            std::runtime_error
        );
        EXPECT_EQ(m.capacity(), cap) << "capacity grew on iteration " << i;
        EXPECT_EQ(m.size(), 0);
    }
}

TYPED_TEST(SlotMapByUnderlyingContainer, Reserve) {
    using Map = SlotMapFor<TypeParam, int>;

    if constexpr (meta::IsReservable<typename TypeParam::template Container<int>>) {
        Map m;

        m.reserve(16);
        const std::size_t reserved_capacity = m.capacity();
        EXPECT_GE(reserved_capacity, 16);
        EXPECT_TRUE(m.empty());

        m.reserve(4);
        EXPECT_GE(m.capacity(), reserved_capacity);

        for (int i = 0; i < 8; ++i) {
            EXPECT_EQ(m.insert(i).index, static_cast<std::size_t>(i));
        }
        EXPECT_EQ(m.size(), 8);
        EXPECT_GE(m.capacity(), reserved_capacity);

        m.erase(3);
        m.erase(6);
        EXPECT_EQ(m.size(), 6);
        EXPECT_GE(m.capacity(), reserved_capacity);

        EXPECT_EQ(m.insert(100).index, 6);
        EXPECT_EQ(m.insert(101).index, 3);
        EXPECT_EQ(m.size(), 8);
        EXPECT_GE(m.capacity(), reserved_capacity);
    }
}

TEST(SlotMap, VectorStorageSpecificCapacity) {
    utils::SlotMap<int, VectorStorage> m;
    EXPECT_GE(m.capacity(), m.size());
    EXPECT_EQ(m.insert(0).index, 0);
    EXPECT_GE(m.capacity(), m.size());

    const std::size_t capacity = m.capacity();
    m.reserve(capacity + 1);
    EXPECT_GE(m.capacity(), capacity + 1);
}

TEST(SlotMap, SmallVectorStorageSpecificCapacity) {
    utils::SlotMap<int, SmallVectorStorage> m;
    const std::size_t inline_capacity = m.capacity();
    EXPECT_GE(inline_capacity, 4);

    m.reserve(2);
    EXPECT_EQ(m.capacity(), inline_capacity);

    for (std::size_t i = 0; i < inline_capacity; ++i) {
        EXPECT_EQ(m.insert(static_cast<int>(i)).index, i);
        EXPECT_EQ(m.capacity(), inline_capacity);
    }

    EXPECT_EQ(m.insert(100).index, inline_capacity);
    EXPECT_GT(m.capacity(), inline_capacity);
}

TYPED_TEST(SlotMapByUnderlyingContainer, ReusesSlotsAfterGrowth) {
    SlotMapFor<TypeParam, int> m;

    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(m.insert(i).index, static_cast<std::size_t>(i));
    }
    const std::size_t grown_capacity = m.capacity();
    EXPECT_GE(grown_capacity, 6);

    m.erase(1);
    m.erase(4);

    EXPECT_EQ(m.insert(100).index, 4);
    EXPECT_EQ(m.insert(101).index, 1);
    EXPECT_EQ(m.capacity(), grown_capacity);
    EXPECT_EQ(m[1], 101);
    EXPECT_EQ(m[4], 100);
}

namespace {

constexpr std::size_t kInitialCount = 8;
constexpr int kInitialBase = 100;
constexpr std::size_t kErasedIndices[]{2, 5, 1};

template <typename Storage>
SlotMapFor<Storage, int> MakeMapWithFreeList() {
    SlotMapFor<Storage, int> m;

    for (std::size_t i = 0; i < kInitialCount; ++i) {
        EXPECT_EQ(m.insert(kInitialBase + static_cast<int>(i)).index, i);
    }

    for (const std::size_t index : kErasedIndices) {
        m.erase(index);
    }
    EXPECT_EQ(m.size(), kInitialCount - std::size(kErasedIndices));

    return m;
}

template <typename Storage>
void ConsumeFreeListFullyAndCheckContents(SlotMapFor<Storage, int>& m, int inserted_base) {
    int next_inserted_value = inserted_base;
    for (const auto expected_reused_index : kErasedIndices | std::views::reverse) {
        const auto result = m.insert(next_inserted_value++);
        EXPECT_EQ(result.index, expected_reused_index);
    }

    EXPECT_EQ(m.size(), kInitialCount);

    {
        const auto result = m.insert(next_inserted_value++);
        EXPECT_EQ(result.index, kInitialCount);
    }

    EXPECT_EQ(m.size(), kInitialCount + 1);

    // Summary of what we've done with the SlotMap:
    //
    // 1. Initial elements are inserted at indices [0;kInitialCount) with values kInitialBase, kInitialBase+1, etc.
    // 2. kErasedIndices are erased (3 elements)
    // 3. 4 elements are reinserted with values inserted_base, inserted_base+1, etc.
    const int expected_by_index[]{
        kInitialBase,
        inserted_base,
        inserted_base + 2,
        kInitialBase + 3,
        kInitialBase + 4,
        inserted_base + 1,
        kInitialBase + 6,
        kInitialBase + 7,
        inserted_base + 3,
    };

    for (std::size_t i = 0; i < std::size(expected_by_index); ++i) {
        EXPECT_EQ(m[i], expected_by_index[i]) << "at index " << i;
    }
}

}  // namespace

TYPED_TEST(SlotMapByUnderlyingContainer, ExpectedFreeListReuse) {
    auto original = MakeMapWithFreeList<TypeParam>();
    ConsumeFreeListFullyAndCheckContents<TypeParam>(original, 200);
}

TYPED_TEST(SlotMapByUnderlyingContainer, CopyConstructionWithFreeList) {
    auto original = MakeMapWithFreeList<TypeParam>();

    auto copied = original;

    ConsumeFreeListFullyAndCheckContents<TypeParam>(original, 200);
    ConsumeFreeListFullyAndCheckContents<TypeParam>(copied, 300);
}

TYPED_TEST(SlotMapByUnderlyingContainer, CopyAssignmentWithFreeList) {
    auto original = MakeMapWithFreeList<TypeParam>();
    auto assigned = MakeMapWithFreeList<TypeParam>();
    assigned.insert(-1);

    assigned = original;

    ConsumeFreeListFullyAndCheckContents<TypeParam>(original, 200);
    ConsumeFreeListFullyAndCheckContents<TypeParam>(assigned, 300);
}

TYPED_TEST(SlotMapByUnderlyingContainer, MoveConstructionWithFreeList) {
    auto original = MakeMapWithFreeList<TypeParam>();

    auto moved = std::move(original);

    ConsumeFreeListFullyAndCheckContents<TypeParam>(moved, 200);
}

TYPED_TEST(SlotMapByUnderlyingContainer, MoveAssignmentWithFreeList) {
    auto original = MakeMapWithFreeList<TypeParam>();
    auto assigned = MakeMapWithFreeList<TypeParam>();
    assigned.insert(-1);

    assigned = std::move(original);

    ConsumeFreeListFullyAndCheckContents<TypeParam>(assigned, 200);
}

USERVER_NAMESPACE_END
