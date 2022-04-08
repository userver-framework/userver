#include <userver/utest/assert_macros.hpp>

#include <stdexcept>
#include <type_traits>

#include <gtest/gtest-spi.h>
#include <boost/algorithm/string/trim.hpp>

#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// To make formatting of code with multiline raw strings palatable
std::string Trim(std::string_view string) {
  return boost::algorithm::trim_copy(std::string{string});
}

void BarrelRoll() {}

template <typename... Args>
void Sink(Args&&... /*args*/) {}

class DummyTracedException : public utils::TracefulException {
 public:
  using utils::TracefulException::TracefulException;
};

void DummyTracedThrowingFunction() { throw DummyTracedException("what"); }

void DummyWrapperFunction() { DummyTracedThrowingFunction(); }

}  // namespace

static_assert(std::is_base_of_v<std::logic_error, std::length_error>);

TEST(AssertMacros, EXPECT_THROW_MSG) {
  UEXPECT_THROW_MSG(throw std::length_error("what"), std::logic_error, "ha");

  EXPECT_NONFATAL_FAILURE(UEXPECT_THROW_MSG(throw std::length_error("what"),
                                            std::logic_error, "foo"),
                          Trim(R"~(
  Expected: 'throw std::length_error("what")' throws 'std::logic_error' with message containing 'foo'.
  Actual: it throws an exception of type 'std::length_error' with message 'what'.
  )~"));

  EXPECT_NONFATAL_FAILURE(UEXPECT_THROW_MSG(throw std::runtime_error("what"),
                                            std::logic_error, "what"),
                          Trim(R"~(
  Expected: 'throw std::runtime_error("what")' throws 'std::logic_error' with message containing 'what'.
  Actual: it throws an exception of a different type 'std::runtime_error' with message 'what'.
  )~"));

  EXPECT_NONFATAL_FAILURE(
      UEXPECT_THROW_MSG(BarrelRoll(), std::runtime_error, "what"), Trim(R"~(
  Expected: 'BarrelRoll()' throws 'std::runtime_error' with message containing 'what'.
  Actual: it does not throw.
  )~"));

  EXPECT_NONFATAL_FAILURE(UEXPECT_THROW_MSG(throw 0, std::logic_error, "what"),
                          "'throw 0' throws a non-std::exception");
}

TEST(AssertMacros, ASSERT_THROW_MSG) {
  UASSERT_THROW_MSG(throw std::length_error("what"), std::logic_error, "ha");

  EXPECT_FATAL_FAILURE(UASSERT_THROW_MSG(throw std::length_error("what"),
                                         std::logic_error, "foo"),
                       Trim(R"~(
  Expected: 'throw std::length_error("what")' throws 'std::logic_error' with message containing 'foo'.
  Actual: it throws an exception of type 'std::length_error' with message 'what'.
  )~"));
}

TEST(AssertMacros, EXPECT_THROW) {
  UEXPECT_THROW(throw std::length_error("what"), std::logic_error);

  EXPECT_NONFATAL_FAILURE(
      UEXPECT_THROW(throw std::runtime_error("what"), std::logic_error),
      Trim(R"~(
  Expected: 'throw std::runtime_error("what")' throws 'std::logic_error'.
  Actual: it throws an exception of a different type 'std::runtime_error' with message 'what'.
  )~"));

  EXPECT_NONFATAL_FAILURE(UEXPECT_THROW(BarrelRoll(), std::runtime_error),
                          Trim(R"~(
  Expected: 'BarrelRoll()' throws 'std::runtime_error'.
  Actual: it does not throw.
  )~"));

  EXPECT_NONFATAL_FAILURE(UEXPECT_THROW(throw 0, std::logic_error),
                          "'throw 0' throws a non-std::exception");
}

TEST(AssertMacros, ASSERT_THROW) {
  UASSERT_THROW(throw std::length_error("what"), std::logic_error);

  EXPECT_FATAL_FAILURE(
      UASSERT_THROW(throw std::runtime_error("what"), std::logic_error),
      Trim(R"~(
  Expected: 'throw std::runtime_error("what")' throws 'std::logic_error'.
  Actual: it throws an exception of a different type 'std::runtime_error' with message 'what'.
  )~"));
}

TEST(AssertMacros, EXPECT_NO_THROW) {
  UEXPECT_NO_THROW(BarrelRoll());

  EXPECT_NONFATAL_FAILURE(UEXPECT_NO_THROW(throw std::runtime_error("what")),
                          Trim(R"~(
  Expected: 'throw std::runtime_error("what")' does not throw.
  Actual: it throws an exception of type 'std::runtime_error' with message 'what'.
  )~"));

  EXPECT_NONFATAL_FAILURE(UEXPECT_NO_THROW(throw 0),
                          "'throw 0' throws a non-std::exception");
}

TEST(AssertMacros, ASSERT_NO_THROW) {
  UASSERT_NO_THROW(BarrelRoll());

  EXPECT_FATAL_FAILURE(UASSERT_NO_THROW(throw std::runtime_error("what")),
                       Trim(R"~(
  Expected: 'throw std::runtime_error("what")' does not throw.
  Actual: it throws an exception of type 'std::runtime_error' with message 'what'.
  )~"));
}

TEST(AssertMacros, LocalVariables) {
  const std::string str = "foo";

  UEXPECT_THROW_MSG(throw std::runtime_error(str), std::runtime_error, "foo");
  UASSERT_THROW_MSG(throw std::runtime_error(str), std::runtime_error, "foo");
  UEXPECT_NO_THROW(Sink(str));
  UASSERT_NO_THROW(Sink(str));
}

TEST(AssertMacros, UserMessage) {
  EXPECT_NONFATAL_FAILURE(UEXPECT_THROW_MSG(throw std::runtime_error("what"),
                                            std::runtime_error, "foo")
                              << "Blah",
                          "Blah");

  EXPECT_NONFATAL_FAILURE(UEXPECT_THROW_MSG(throw std::runtime_error("what"),
                                            std::logic_error, "what")
                              << "Blah",
                          "Blah");

  EXPECT_NONFATAL_FAILURE(
      UEXPECT_THROW_MSG(BarrelRoll(), std::runtime_error, "what") << "Blah",
      "Blah");

  EXPECT_NONFATAL_FAILURE(
      UEXPECT_NO_THROW(throw std::runtime_error("what")) << "Blah", "Blah");
}

TEST(AssertMacros, TracefulException) {
  logging::stacktrace_cache::StacktraceGuard guard(true);

  EXPECT_NONFATAL_FAILURE(
      UEXPECT_THROW(DummyWrapperFunction(), std::runtime_error),
      Trim("with message 'what' and trace:\n 0# "));

  EXPECT_NONFATAL_FAILURE(
      UEXPECT_THROW(DummyWrapperFunction(), std::runtime_error),
      "DummyTracedThrowingFunction");
}

namespace {

/// [Sample assert macros usage]
void ThrowingFunction() { throw std::runtime_error("The message"); }

void NonThrowingFunction() {}

TEST(AssertMacros, Sample) {
  UEXPECT_THROW_MSG(ThrowingFunction(), std::runtime_error, "message");
  UEXPECT_THROW(ThrowingFunction(), std::runtime_error);
  UEXPECT_THROW(ThrowingFunction(), std::exception);
  UEXPECT_NO_THROW(NonThrowingFunction());
}
/// [Sample assert macros usage]

}  // namespace

TEST(AssertMacros, EXPECT_UINVARIANT_FAILURE) {
  EXPECT_UINVARIANT_FAILURE(UINVARIANT(false, "what"));
  EXPECT_NONFATAL_FAILURE(EXPECT_UINVARIANT_FAILURE(BarrelRoll()), "");
}

USERVER_NAMESPACE_END
