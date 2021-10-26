#pragma once

/// @file userver/utest/parameter_names.hpp
/// @brief @copybrief utest::PrintTestName

#include <string>
#include <tuple>
#include <type_traits>

#include <gtest/gtest.h>

#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace impl {

template <typename T>
using HasTestName = decltype(T::test_name);

template <typename ParamType>
std::string TestParamToString(const ParamType& param) {
  if constexpr (meta::kIsDetected<HasTestName, ParamType>) {
    static_assert(std::is_same_v<std::string, decltype(param.test_name)>,
                  "Test name should be a string");
    return param.test_name;
  } else {
    return ::testing::PrintToString(param);
  }
}

template <typename T>
using HasTupleSize = decltype(std::tuple_size<T>::value);

template <typename... Args>
std::string TestTupleParamToString(const std::tuple<Args...>& params_tuple) {
  static_assert((sizeof...(Args) != 0),
                "Test parameters should have at least one dimension");
  // Underscore is the only valid non-alphanumeric character that can be used
  // as a separator, see IsValidParamName in gtest-param-util.h.
  return std::apply(
      [](const auto& first, const auto&... rest) {
        return (TestParamToString(first) + ... +
                ("_" + TestParamToString(rest)));
      },
      params_tuple);
}

}  // namespace impl

// clang-format off

/// @ingroup userver_utest
///
/// @brief Test name printer for parameterized tests written in gtest.
///
/// ## Example usage:
///
/// ### Singly-parameterized test.
///
/// Special-purpose field `test_name` of type `std::string` may be used for describing test input in a human-readable form.
///
/// @snippet core/testing/src/utest/parameter_names_test.cpp  PrintTestName Example Usage - Singly-Parameterized Test
///
/// This should result in printing `test_name` field as a test name during test run.
///
/// ```
/// ParametrizedTest.BasicTest/First
/// ParametrizedTest.BasicTest/Second
/// ParametrizedTest.BasicTest/Third
/// ```
///
/// ### Another option to override a test name.
///
/// Helper class `::utest::PrintTestName()` also supports conventional `PrintTo` override.
/// However field `test_name` of a parameters' structure has a higher priority for overriding a test name than a `PrintTo` function.
///
/// @snippet core/testing/src/utest/parameter_names_test.cpp  PrintTestName Example Usage - Override PrintTo
///
/// ### Doubly-parametrized test.
///
/// In case you have more than one dimension for possible test parameters,
/// you can also use `::utest::PrintTestName()` to combine names for the dimensions of every parameter.
/// You can mix methods for overriding test names for the dimension of every parameter independently.
///
/// @snippet core/testing/src/utest/parameter_names_test.cpp  PrintTestName Example Usage - Doubly-Parameterized Test
///
/// This should result in printing concatenated test names for all combined test parameters dimensions.
///
/// ```
/// DoublyParametrizedTest.BasicTest/First_CustomFirst
/// DoublyParametrizedTest.BasicTest/First_CustomSecond
/// DoublyParametrizedTest.BasicTest/Second_CustomFirst
/// DoublyParametrizedTest.BasicTest/Second_CustomSecond
/// DoublyParametrizedTest.BasicTest/Third_CustomFirst
/// DoublyParametrizedTest.BasicTest/Third_CustomSecond
/// ```

// clang-format on
struct PrintTestName final {
  template <typename ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType>& info) const {
    if constexpr (meta::kIsDetected<impl::HasTupleSize, ParamType>) {
      return impl::TestTupleParamToString(info.param);
    } else {
      return impl::TestParamToString(info.param);
    }
  }
};

}  // namespace utest

USERVER_NAMESPACE_END
