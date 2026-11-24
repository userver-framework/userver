#include <concurrent/impl/locked_list.hpp>

#include <cstdint>

#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

TEST(DoublyLinkedList, IndependentHooks) {
    struct Node {
        DoublyLinkedHook<Node> primary;
        DoublyLinkedHook<Node> secondary;
    };

    Node a, b, c;
    DoublyLinkedList<Node, MemberHook<&Node::primary>> primary;
    DoublyLinkedList<Node, MemberHook<&Node::secondary>> secondary;
    primary.PushFront(a);
    primary.PushFront(b);
    primary.PushFront(c);
    secondary.PushFront(b);
    secondary.PushFront(a);

    ASSERT_FALSE(primary.IsEmpty());
    primary.Erase(b);
    EXPECT_EQ(c.primary.next, &a);
    EXPECT_EQ(a.primary.prev, &c);
    primary.PushFront(b);
    EXPECT_EQ(&primary.PopFront(), &b);
    EXPECT_EQ(b.primary.next, nullptr);
    EXPECT_EQ(c.primary.prev, nullptr);
    EXPECT_EQ(&primary.PopFront(), &c);
    EXPECT_EQ(&primary.PopFront(), &a);
    EXPECT_TRUE(primary.IsEmpty());
    EXPECT_EQ(&secondary.PopFront(), &a);
    EXPECT_EQ(&secondary.PopFront(), &b);
    EXPECT_TRUE(secondary.IsEmpty());
}

TEST(LockedList, PublishesHeadOnUnlock) {
    struct Node {
        DoublyLinkedHook<Node> hook;
    };
    using Guard = LockedList<Node, MemberHook<&Node::hook>>;

    Node node;
    std::intptr_t shard{0};
    {
        Guard locked{shard};
        ASSERT_TRUE(locked.GetList().IsEmpty());
        locked.GetList().PushFront(node);
    }
    EXPECT_EQ(shard, reinterpret_cast<std::intptr_t>(&node));
    {
        Guard locked{shard};
        ASSERT_EQ(locked.GetList().GetHead(), &node);
        locked.GetList().PopFront();
    }
    EXPECT_EQ(shard, 0);
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
