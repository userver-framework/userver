#include <engine/async.hpp>
#include <engine/condition_variable.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <utest/utest.hpp>

#include <atomic>

namespace {

struct OverloadsTest {
  std::size_t overload1{0};
  std::size_t overload2{0};
  std::size_t overload3{0};

  mutable std::size_t overload4{0};
  mutable std::size_t overload5{0};
  mutable std::size_t overload6{0};

  static std::size_t overload7;
  static std::size_t overload8;
  static std::size_t overload9;

  void operator()(std::string&, std::string&&) & { ++overload1; }
  void operator()(const std::string&, std::string&&) & { ++overload2; }
  void operator()(const std::string&, const std::string&) & { ++overload3; }

  void operator()(std::string&, std::string&&) const & { ++overload4; }
  void operator()(const std::string&, std::string&&) const & { ++overload5; }
  void operator()(const std::string&, const std::string&) const & {
    ++overload6;
  }

  void operator()(std::string&, std::string&&) && { ++overload7; }
  void operator()(const std::string&, std::string&&) && { ++overload8; }
  void operator()(const std::string&, const std::string&) && { ++overload9; }
};

std::size_t OverloadsTest::overload7{0};
std::size_t OverloadsTest::overload8{0};
std::size_t OverloadsTest::overload9{0};

struct CountingConstructions {
  static int constructions;
  static int destructions;

  CountingConstructions() { ++constructions; }
  CountingConstructions(CountingConstructions&&) noexcept { ++constructions; }
  CountingConstructions(const CountingConstructions&) { ++constructions; }

  void operator()() const { /*noop*/
  }

  CountingConstructions& operator=(CountingConstructions&&) = default;
  CountingConstructions& operator=(const CountingConstructions&) = default;

  ~CountingConstructions() { ++destructions; }
};

int CountingConstructions::constructions = 0;
int CountingConstructions::destructions = 0;

}  // namespace

TEST(Async, OverloadSelection) {
  RunInCoro([] {
    OverloadsTest tst;
    const std::string const_string{};
    std::string nonconst_string{};

    engine::Async(std::ref(tst), nonconst_string, std::string{}).Wait();
    EXPECT_EQ(tst.overload1, 1);

    engine::Async(std::ref(tst), const_string, std::string{}).Wait();
    EXPECT_EQ(tst.overload2, 1);

    engine::Async(std::ref(tst), const_string, const_string).Wait();
    EXPECT_EQ(tst.overload3, 1);

    engine::Async(std::cref(tst), nonconst_string, std::string{}).Wait();
    EXPECT_EQ(tst.overload4, 1);

    engine::Async(std::cref(tst), const_string, std::string{}).Wait();
    EXPECT_EQ(tst.overload5, 1);

    engine::Async(std::cref(tst), const_string, const_string).Wait();
    EXPECT_EQ(tst.overload6, 1);

    engine::Async(std::move(tst), nonconst_string, std::string{}).Wait();
    EXPECT_EQ(tst.overload7, 1);

    engine::Async(std::move(tst), const_string, std::string{}).Wait();
    EXPECT_EQ(tst.overload8, 1);

    engine::Async(std::move(tst), const_string, const_string).Wait();
    EXPECT_EQ(tst.overload9, 1);

    auto* function_ptr = +[]() {};
    static_assert(std::is_pointer<decltype(function_ptr)>::value, "");
    EXPECT_NO_THROW(engine::Async(function_ptr).Wait());
  });
}

TEST(Async, ResourceDeallocation) {
  RunInCoro([] {
    engine::Async(CountingConstructions{}).Wait();
    EXPECT_EQ(CountingConstructions::constructions,
              CountingConstructions::destructions);
  });
}
