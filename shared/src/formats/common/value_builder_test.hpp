#pragma once

#include <limits>
#include <type_traits>

#include <gtest/gtest.h>

#include <userver/compiler/demangle.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/utest/death_tests.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

template <class T>
struct InstantiationDeathTest : public ::testing::Test {};
TYPED_TEST_SUITE_P(InstantiationDeathTest);

template <class T>
struct CommonValueBuilderTests : public ::testing::Test {
  using ValueBuilder = T;
};
TYPED_TEST_SUITE_P(CommonValueBuilderTests);

template <typename Float, typename ValueBuilder, typename Exception>
inline void TestNanInfInstantiation() {
// In debug builds we UASSERT for Nan/Inf
#ifdef NDEBUG
  ASSERT_THROW(ValueBuilder{std::numeric_limits<Float>::signaling_NaN()},
               Exception)
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  ASSERT_THROW(ValueBuilder{std::numeric_limits<Float>::quiet_NaN()}, Exception)
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  ASSERT_THROW(ValueBuilder{std::numeric_limits<Float>::infinity()}, Exception)
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  ASSERT_THROW(ValueBuilder{-std::numeric_limits<Float>::infinity()}, Exception)
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
#else
  UEXPECT_DEATH(ValueBuilder{std::numeric_limits<Float>::signaling_NaN()}, "")
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  UEXPECT_DEATH(ValueBuilder{std::numeric_limits<Float>::quiet_NaN()}, "")
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  UEXPECT_DEATH(ValueBuilder{std::numeric_limits<Float>::infinity()}, "")
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  UEXPECT_DEATH(ValueBuilder{-std::numeric_limits<Float>::infinity()}, "")
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
#endif
}

template <typename, typename, typename = std::void_t<> >
struct CanCallOperatorBrackets : public std::false_type {};

template <typename T, typename U>
struct CanCallOperatorBrackets<
    T, U, std::void_t<decltype(std::declval<T>()[std::declval<U>()])> >
    : std::true_type {};

namespace formats::bson {
// this is part of an ugly hack to overcome compile error when constructing
// bson::ValueBuilder(float)
class ValueBuilder;
}  // namespace formats::bson

TYPED_TEST_P(InstantiationDeathTest, NanInf) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using Exception = typename TestFixture::Exception;

  TestNanInfInstantiation<double, ValueBuilder, Exception>();
  if constexpr (!std::is_same<formats::bson::ValueBuilder,
                              ValueBuilder>::value) {
    // this won't compile for bson
    TestNanInfInstantiation<float, ValueBuilder, Exception>();
  }
}

REGISTER_TYPED_TEST_SUITE_P(InstantiationDeathTest, NanInf);

TYPED_TEST_P(CommonValueBuilderTests, StringStrongTypedef) {
  using ValueBuilder = typename TestFixture::ValueBuilder;

  using MyStringTypedef = utils::StrongTypedef<struct Tag, std::string>;

  // using as key
  {
    ValueBuilder builder{formats::common::Type::kObject};
    builder[MyStringTypedef{"test_key"}] = 7;

    ASSERT_EQ(7, builder.ExtractValue()["test_key"].template As<int>());
  }

  // using in constructor
  {
    ValueBuilder builder{MyStringTypedef{"test_string"}};

    ASSERT_EQ("test_string", builder.ExtractValue().template As<std::string>());
  }

  // non-loggable can't be used
  {
    using MyNonLoggableStringTypedef =
        utils::NonLoggable<struct Tag, std::string>;
    static_assert(
        (CanCallOperatorBrackets<ValueBuilder, MyStringTypedef>::value));
    static_assert(
        !(CanCallOperatorBrackets<ValueBuilder,
                                  MyNonLoggableStringTypedef>::value));
  }
}

TYPED_TEST_P(CommonValueBuilderTests, Resize) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  ValueBuilder builder;
  constexpr std::size_t new_size = 10;
  builder.Resize(new_size);
  for (std::size_t i = 0; i < new_size; ++i) {
    builder[i] = i;
  }
  const auto value = builder.ExtractValue();
  for (std::size_t i = 0; i < new_size; ++i) {
    EXPECT_EQ(i, value[i].template As<std::size_t>());
  }
}

REGISTER_TYPED_TEST_SUITE_P(CommonValueBuilderTests, StringStrongTypedef,
                            Resize);

USERVER_NAMESPACE_END
