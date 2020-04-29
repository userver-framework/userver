#include <limits>
#include <type_traits>

#include <gtest/gtest.h>
#include <compiler/demangle.hpp>

namespace {

template <class T>
struct Instantiation : public ::testing::Test {};
TYPED_TEST_SUITE_P(Instantiation);

template <typename Float, typename ValueBuilder, typename Exception>
void TestNanInfInstantiation() {
  ASSERT_THROW(ValueBuilder{std::numeric_limits<Float>::signaling_NaN()},
               Exception)
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  ASSERT_THROW(ValueBuilder{std::numeric_limits<Float>::quiet_NaN()}, Exception)
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  ASSERT_NO_THROW(ValueBuilder{std::numeric_limits<Float>::infinity()})
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
  ASSERT_NO_THROW(ValueBuilder{-std::numeric_limits<Float>::infinity()})
      << "Assertion failed for type " << compiler::GetTypeName<Float>();
}
}  // namespace

namespace formats::bson {
// this is part of an ugly hack to overcome compile error when constructing
// bson::ValueBuilder(float)
class ValueBuilder;
}  // namespace formats::bson

TYPED_TEST_P(Instantiation, NanInf) {
  using ValueBuilder = typename TestFixture::ValueBuilder;
  using Exception = typename TestFixture::Exception;

  TestNanInfInstantiation<double, ValueBuilder, Exception>();
  if constexpr (!std::is_same<formats::bson::ValueBuilder,
                              ValueBuilder>::value) {
    // this won't compile for bson
    TestNanInfInstantiation<float, ValueBuilder, Exception>();
  }
}

REGISTER_TYPED_TEST_SUITE_P(Instantiation, NanInf);
