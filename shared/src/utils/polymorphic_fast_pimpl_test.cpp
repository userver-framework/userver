#include <gtest/gtest.h>

#include <utils/polymorphic_fast_pimpl.hpp>

namespace {

class Base {
 public:
  virtual ~Base() = default;
};

class Derived : public Base {
 public:
  explicit Derived(bool& is_alive) : is_alive_(is_alive) { is_alive_ = true; }

  ~Derived() { is_alive_ = false; }

 private:
  bool& is_alive_;
};

class DerivedNoMembers : public Base {
 public:
  int Get42() { return 42; }
};

}  // namespace

TEST(PolymorphicFastPimpl, Smoke) {
  utils::PolymorphicFastPimpl<Base, 8, 8, true> test{std::in_place_type<Base>};
}

TEST(PolymorphicFastPimpl, Strict) {
  utils::PolymorphicFastPimpl<Base, 8, 8, true> test{
      std::in_place_type<DerivedNoMembers>};
  EXPECT_EQ(42, dynamic_cast<DerivedNoMembers*>(test.Get())->Get42());
}

TEST(PolymorphicFastPimpl, VirtualDtor) {
  bool is_derived_alive = false;

  {
    utils::PolymorphicFastPimpl<Base, 16, 8, false> test{
        std::in_place_type<Derived>, is_derived_alive};
    EXPECT_TRUE(is_derived_alive);
  }
  EXPECT_FALSE(is_derived_alive);
}
