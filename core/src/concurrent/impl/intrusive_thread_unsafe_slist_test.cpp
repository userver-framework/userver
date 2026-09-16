#include <userver/concurrent/impl/intrusive_thread_unsafe_slist.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <vector>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Node final : public concurrent::impl::SinglyLinkedBaseHook {
    explicit Node(int value = 0)
        : value(value)
    {}

    int value{0};
};

struct NoOpCountingDeleter {
    static inline std::size_t deleted_count{0};

    void operator()(Node*) const noexcept { ++deleted_count; }
};

using OwnedSlist = concurrent::impl::ThreadUnsafeSlist<Node>;
using ViewSlist = concurrent::impl::ThreadUnsafeSlist<Node, NoOpCountingDeleter>;

static_assert(std::forward_iterator<OwnedSlist::iterator>);
static_assert(std::forward_iterator<ViewSlist::iterator>);

std::vector<int> CollectValues(ViewSlist& list) {
    std::vector<int> values;
    for (auto& item : list) {
        values.push_back(item.value);
    }
    return values;
}

class ThreadUnsafeSlist : public ::testing::Test {
protected:
    void SetUp() override { NoOpCountingDeleter::deleted_count = 0; }
};

}  // namespace

TEST_F(ThreadUnsafeSlist, Empty) {
    OwnedSlist list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.begin(), list.end());
}

TEST_F(ThreadUnsafeSlist, AdoptSingle) {
    OwnedSlist list;
    auto* node = std::make_unique<Node>(1).release();

    auto it = list.Adopt(list.end(), node);

    EXPECT_FALSE(list.empty());
    EXPECT_EQ(it, list.begin());
    EXPECT_NE(list.begin(), list.end());
    EXPECT_EQ(it->value, 1);
    EXPECT_EQ((*it).value, 1);
    EXPECT_EQ(it.GetNodeRawPointer(), node);

    auto next = it;
    ++next;
    EXPECT_EQ(next, list.end());
}

TEST_F(ThreadUnsafeSlist, AdoptAppendsInOrder) {
    Node a{1};
    Node b{2};
    Node c{3};
    ViewSlist list;

    auto last = list.begin();
    last = list.Adopt(last, &a);
    last = list.Adopt(last, &b);
    last = list.Adopt(last, &c);

    EXPECT_FALSE(list.empty());
    EXPECT_EQ(CollectValues(list), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(last.GetNodeRawPointer(), &c);
    EXPECT_EQ(NoOpCountingDeleter::deleted_count, 0);

    list.EraseFromBegin(list.end());
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(NoOpCountingDeleter::deleted_count, 3);
}

TEST_F(ThreadUnsafeSlist, IteratorPostIncrement) {
    Node a{10};
    Node b{20};
    ViewSlist list;

    auto last = list.begin();
    last = list.Adopt(last, &a);
    list.Adopt(last, &b);

    auto it = list.begin();
    auto prev = it++;
    EXPECT_EQ(prev->value, 10);
    EXPECT_EQ(it->value, 20);

    prev = it++;
    EXPECT_EQ(prev->value, 20);
    EXPECT_EQ(it, list.end());

    list.EraseFromBegin(list.end());
    EXPECT_EQ(NoOpCountingDeleter::deleted_count, 2);
}

TEST_F(ThreadUnsafeSlist, EraseFromBeginPartial) {
    Node a{1};
    Node b{2};
    Node c{3};
    ViewSlist list;

    auto last = list.begin();
    last = list.Adopt(last, &a);
    last = list.Adopt(last, &b);
    last = list.Adopt(last, &c);

    auto keep = list.begin();
    ++keep;
    ++keep;  // points to node with value 3

    list.EraseFromBegin(keep);

    EXPECT_EQ(NoOpCountingDeleter::deleted_count, 2);
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.begin()->value, 3);

    auto it = list.begin();
    ++it;
    EXPECT_EQ(it, list.end());
}

TEST_F(ThreadUnsafeSlist, Destructor) {
    {
        Node a{1};
        Node b{2};
        ViewSlist list;

        auto last = list.begin();
        last = list.Adopt(last, &a);
        last = list.Adopt(last, &b);
    }
    EXPECT_EQ(NoOpCountingDeleter::deleted_count, 2);
}

TEST_F(ThreadUnsafeSlist, EraseFromBeginEmptyRange) {
    Node a{1};
    Node b{2};
    ViewSlist list;

    auto last = list.begin();
    last = list.Adopt(last, &a);
    last = list.Adopt(last, &b);

    list.EraseFromBegin(list.begin());

    EXPECT_EQ(NoOpCountingDeleter::deleted_count, 0);
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.begin()->value, 1);
}

TEST_F(ThreadUnsafeSlist, AdoptAfterEraseContinuesFromRemainder) {
    Node a{1};
    Node b{2};
    Node c{3};
    Node d{4};
    ViewSlist list;

    auto last = list.begin();
    last = list.Adopt(last, &a);
    last = list.Adopt(last, &b);
    last = list.Adopt(last, &c);

    auto keep = list.begin();
    ++keep;
    list.EraseFromBegin(keep);  // drop a, keep b -> c

    EXPECT_EQ(CollectValues(list), (std::vector<int>{2, 3}));

    last = list.begin();
    ++last;  // c
    list.Adopt(last, &d);

    EXPECT_EQ(CollectValues(list), (std::vector<int>{2, 3, 4}));
}

TEST_F(ThreadUnsafeSlist, IteratorEquality) {
    Node a{1};
    Node b{2};
    ViewSlist list;

    auto last = list.begin();
    last = list.Adopt(last, &a);
    list.Adopt(last, &b);

    auto it1 = list.begin();
    auto it2 = list.begin();
    EXPECT_EQ(it1, it2);

    ++it2;
    EXPECT_NE(it1, it2);
    EXPECT_NE(it2, list.end());

    ++it2;
    EXPECT_EQ(it2, list.end());

    list.EraseFromBegin(list.end());
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.begin(), list.end());
}

USERVER_NAMESPACE_END
