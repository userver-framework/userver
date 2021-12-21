#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include <userver/utils/get_if.hpp>

USERVER_NAMESPACE_BEGIN

TEST(GetIf, Basic) {
  struct B {};
  struct A {
    B b;
  };

  {
    A a;
    EXPECT_EQ((utils::GetIf<B, &A::b>(a)), &a.b);
  }

  {
    A a;
    auto p = &a;
    EXPECT_EQ((utils::GetIf<B, &A::b>(p)), &a.b);
  }

  {
    A* a{};
    EXPECT_EQ((utils::GetIf<B, &A::b>(a)), static_cast<B*>(nullptr));
  }

  {
    auto a = std::make_optional<A>();
    EXPECT_EQ((utils::GetIf<B, &A::b>(a)), &a->b);
  }

  {
    std::optional<A> a;
    EXPECT_EQ((utils::GetIf<B, &A::b>(a)), static_cast<B*>(nullptr));
  }

  {
    auto a = std::make_unique<A>();
    EXPECT_EQ((utils::GetIf<B, &A::b>(a)), &a->b);
  }

  {
    std::unique_ptr<A> a;
    EXPECT_EQ((utils::GetIf<B, &A::b>(a)), static_cast<B*>(nullptr));
  }

  {
    auto a = std::make_shared<A>();
    EXPECT_EQ((utils::GetIf<B, &A::b>(a)), &a->b);
  }

  {
    std::shared_ptr<A> a;
    EXPECT_EQ((utils::GetIf<B, &A::b>(a)), static_cast<B*>(nullptr));
  }
}

TEST(GetIf, Chain) {
  /// [Sample GetIf Usage]
  struct C {};
  struct B {
    std::unique_ptr<C> c = std::make_unique<C>();
  };
  struct A {
    std::shared_ptr<B> b = std::make_shared<B>();
  };

  auto a = std::make_optional<A>();
  EXPECT_EQ((utils::GetIf<C, &A::b, &B::c>(a)), a->b->c.get());
  EXPECT_EQ((utils::GetIf<B, &A::b>(a)), a->b.get());
  EXPECT_EQ((utils::GetIf<A>(a)), &(*a));
  a->b->c.reset();
  EXPECT_EQ((utils::GetIf<C, &A::b, &B::c>(a)), a->b->c.get());
  EXPECT_EQ((utils::GetIf<B, &A::b>(a)), a->b.get());
  EXPECT_EQ((utils::GetIf<A>(a)), &(*a));
  a->b.reset();
  EXPECT_EQ((utils::GetIf<C, &A::b, &B::c>(a)),
            static_cast<decltype(a->b->c.get())>(nullptr));
  EXPECT_EQ((utils::GetIf<B, &A::b>(a)), a->b.get());
  EXPECT_EQ((utils::GetIf<A>(a)), &(*a));
  a.reset();
  EXPECT_EQ((utils::GetIf<C, &A::b, &B::c>(a)),
            static_cast<decltype(a->b->c.get())>(nullptr));
  EXPECT_EQ((utils::GetIf<B, &A::b>(a)),
            static_cast<decltype(a->b.get())>(nullptr));
  EXPECT_EQ((utils::GetIf<A>(a)), static_cast<decltype(&(*a))>(nullptr));
  /// [Sample GetIf Usage]
}

TEST(GetIf, DoubleIndirection) {
  struct B {};
  struct A {
    std::optional<std::optional<B>> b = B{};
  };

  A a;
  EXPECT_EQ((utils::GetIf<B, &A::b>(a)), &(**a.b));
  a.b->reset();
  EXPECT_EQ((utils::GetIf<B, &A::b>(a)),
            static_cast<decltype(&(**a.b))>(nullptr));
  a.b.reset();
  EXPECT_EQ((utils::GetIf<B, &A::b>(a)),
            static_cast<decltype(&(**a.b))>(nullptr));
}

TEST(GetIf, RawPtr) {
  struct B {};
  struct A {
    B* b;
  };

  B b;
  A a{&b};
  EXPECT_EQ((utils::GetIf<B, &A::b>(a)), &b);
}

USERVER_NAMESPACE_END
