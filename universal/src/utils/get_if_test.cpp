#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include <userver/utils/get_if.hpp>

USERVER_NAMESPACE_BEGIN

TEST(GetIf, IsPointerLike) {
  static_assert(!utils::impl::kIsPointerLike<int>);
  static_assert(!utils::impl::kIsPointerLike<int&>);
  static_assert(!utils::impl::kIsPointerLike<std::optional<int>>);
  static_assert(!utils::impl::kIsPointerLike<std::nullptr_t>);

  static_assert(utils::impl::kIsPointerLike<std::optional<int>&>);

  static_assert(utils::impl::kIsPointerLike<std::shared_ptr<int>>);
  static_assert(utils::impl::kIsPointerLike<std::shared_ptr<int>&>);

  static_assert(utils::impl::kIsPointerLike<std::unique_ptr<int>>);
  static_assert(utils::impl::kIsPointerLike<std::unique_ptr<int>&>);

  static_assert(utils::impl::kIsPointerLike<int*>);
  static_assert(utils::impl::kIsPointerLike<int**>);
  static_assert(utils::impl::kIsPointerLike<int*&>);
}

TEST(GetIf, Basic) {
  struct B {};
  struct A {
    B b;
  };

  {
    A a{};
    EXPECT_EQ(utils::GetIf(a, &A::b), &a.b);
  }

  {
    A a{};
    EXPECT_EQ(utils::GetIf(std::ref(a), &A::b), &a.b);
  }

  {
    A a{};
    EXPECT_EQ(utils::GetIf(&a, &A::b), &a.b);
  }

  {
    A* a = nullptr;
    EXPECT_EQ(utils::GetIf(a, &A::b), static_cast<B*>(nullptr));
  }

  {
    auto a = std::make_optional<A>();
    EXPECT_EQ(utils::GetIf(a, &A::b), &a->b);
  }

  {
    std::optional<A> a;
    EXPECT_EQ(utils::GetIf(a, &A::b), static_cast<B*>(nullptr));
  }

  {
    auto a = std::make_unique<A>();
    EXPECT_EQ(utils::GetIf(a, &A::b), &a->b);
  }

  {
    std::unique_ptr<A> a;
    EXPECT_EQ(utils::GetIf(a, &A::b), static_cast<B*>(nullptr));
  }

  {
    auto a = std::make_shared<A>();
    EXPECT_EQ(utils::GetIf(a, &A::b), &a->b);
  }

  {
    std::shared_ptr<A> a;
    EXPECT_EQ(utils::GetIf(a, &A::b), static_cast<B*>(nullptr));
  }
}

TEST(GetIf, Chain) {
  /// [Sample Usage]
  struct C {};
  struct B {
    std::unique_ptr<C> c = std::make_unique<C>();
  };
  struct A {
    std::shared_ptr<B> b = std::make_shared<B>();
  };

  auto a = std::make_optional<A>();
  EXPECT_EQ(utils::GetIf(a, &A::b, &B::c), a->b->c.get());
  EXPECT_EQ(utils::GetIf(a, &A::b), a->b.get());
  EXPECT_EQ(utils::GetIf(a), &*a);
  a->b->c.reset();
  EXPECT_EQ(utils::GetIf(a, &A::b, &B::c), static_cast<C*>(nullptr));
  EXPECT_EQ(utils::GetIf(a, &A::b), a->b.get());
  EXPECT_EQ(utils::GetIf(a), &*a);
  a->b.reset();
  EXPECT_EQ(utils::GetIf(a, &A::b, &B::c), static_cast<C*>(nullptr));
  EXPECT_EQ(utils::GetIf(a, &A::b), static_cast<B*>(nullptr));
  EXPECT_EQ(utils::GetIf(a), &*a);
  a.reset();
  EXPECT_EQ(utils::GetIf(a, &A::b, &B::c), static_cast<C*>(nullptr));
  EXPECT_EQ(utils::GetIf(a, &A::b), static_cast<B*>(nullptr));
  EXPECT_EQ(utils::GetIf(a), static_cast<A*>(nullptr));
  /// [Sample Usage]
}

TEST(GetIf, DoubleIndirection) {
  struct B {};
  struct A {
    std::optional<std::optional<B>> b = B{};
  };

  {
    A a;
    EXPECT_EQ(utils::GetIf(a, &A::b), &**a.b);
    a.b->reset();
    EXPECT_EQ(utils::GetIf(a, &A::b), static_cast<B*>(nullptr));
  }

  {
    A a;
    EXPECT_EQ(utils::GetIf(a, &A::b), &**a.b);
    a.b.reset();
    EXPECT_EQ(utils::GetIf(a, &A::b), static_cast<B*>(nullptr));
  }
}

TEST(GetIf, Unary) {
  struct A {};

  A a;

  {
    auto p = std::make_optional(
        std::make_unique<std::shared_ptr<A*>>(std::make_shared<A*>(&a)));
    ASSERT_EQ(utils::GetIf(p), &a);
    ***p = nullptr;
    ASSERT_TRUE(p && *p && **p && !***p);
    EXPECT_EQ(utils::GetIf(p), static_cast<A*>(nullptr));
  }

  {
    auto p = std::make_optional(
        std::make_unique<std::shared_ptr<A*>>(std::make_shared<A*>(&a)));
    ASSERT_EQ(utils::GetIf(p), &a);
    (*p)->reset();
    ASSERT_TRUE(p && *p && !**p);
    EXPECT_EQ(utils::GetIf(p), static_cast<A*>(nullptr));
  }

  {
    auto p = std::make_optional(
        std::make_unique<std::shared_ptr<A*>>(std::make_shared<A*>(&a)));
    ASSERT_EQ(utils::GetIf(p), &a);
    p->reset();
    ASSERT_TRUE(p && !*p);
    EXPECT_EQ(utils::GetIf(p), static_cast<A*>(nullptr));
  }

  {
    auto p = std::make_optional(
        std::make_unique<std::shared_ptr<A*>>(std::make_shared<A*>(&a)));
    ASSERT_EQ(utils::GetIf(p), &a);
    p.reset();
    ASSERT_TRUE(!p);
    EXPECT_EQ(utils::GetIf(p), static_cast<A*>(nullptr));
  }
}

TEST(GetIf, RawPtr) {
  struct B {};
  struct A {
    B* b;
  };

  B b;
  A a{&b};
  EXPECT_EQ(utils::GetIf(a, &A::b), &b);
}

TEST(GetIf, FreeFunction) {
  struct B {};
  struct A {
    B b;
  };

  auto g = [](A& a) -> B& { return a.b; };
  A a{};
  EXPECT_EQ(utils::GetIf(a, g), &a.b);
  EXPECT_EQ(utils::GetIf(a, std::ref(g)), &a.b);

  using FuncPtr = B& (*)(A&);
  FuncPtr f = g;
  EXPECT_EQ(utils::GetIf(a, f), &a.b);
  EXPECT_EQ(utils::GetIf(a, std::ref(f)), &a.b);
}

TEST(GetIf, PerfectForwarding) {
  struct C {};
  struct B {};
  struct A {
    C c[4];

    C& operator()(B&) { return c[0]; }
    C& operator()(const B&) { return c[1]; }
    C& operator()(B&&) { return c[2]; }
    C& operator()(const B&&) { return c[3]; }
  };

  A a{};
  B b;
  EXPECT_EQ(utils::GetIf(b, a), a.c + 0);
  EXPECT_EQ(utils::GetIf(std::as_const(b), a), a.c + 1);
  EXPECT_EQ(utils::GetIf(std::move(b), a), a.c + 2);
  EXPECT_EQ(utils::GetIf(std::move(std::as_const(b)), a), a.c + 3);
}

USERVER_NAMESPACE_END
