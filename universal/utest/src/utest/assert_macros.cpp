#include <userver/utest/assert_macros.hpp>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest::impl {

namespace {

std::string WithExpectedMessage(std::string_view expected_message) {
  return expected_message.empty()
             ? std::string{}
             : fmt::format(" with message containing '{}'", expected_message);
}

std::string WithActualMessage(const std::exception& exception) {
  const auto* traceful =
      dynamic_cast<const utils::TracefulExceptionBase*>(&exception);
  if (traceful) {
    const auto& message_buffer = traceful->MessageBuffer();
    return fmt::format(
        " with message '{}' and trace:\n{}",
        std::string_view(message_buffer.data(), message_buffer.size()),
        logging::stacktrace_cache::to_string(traceful->Trace()));
  } else {
    return fmt::format(" with message '{}'. ", exception.what());
  }
}

std::string MakeMismatchedExceptionMessage(std::string_view statement,
                                           const std::exception& exception,
                                           const std::type_info& expected_type,
                                           std::string_view expected_message) {
  UASSERT(!expected_message.empty());
  return fmt::format(
      "  Expected: '{}' throws '{}' with message containing '{}'.\n"
      "  Actual: it throws an exception of type '{}'{}",
      statement, compiler::GetTypeName(expected_type), expected_message,
      compiler::GetTypeName(typeid(exception)), WithActualMessage(exception));
}

std::string MakeMismatchedExceptionTypeMessage(
    std::string_view statement, const std::exception& exception,
    const std::type_info& expected_type, std::string_view expected_message) {
  return fmt::format(
      "  Expected: '{}' throws '{}'{}.\n"
      "  Actual: it throws an exception of a different type '{}'{}",
      statement, compiler::GetTypeName(expected_type),
      WithExpectedMessage(expected_message),
      compiler::GetTypeName(typeid(exception)), WithActualMessage(exception));
}

std::string MakeThrownExceptionMessage(std::string_view statement,
                                       const std::exception& exception) {
  return fmt::format(
      "  Expected: '{}' does not throw.\n"
      "  Actual: it throws an exception of type '{}'{}",
      statement, compiler::GetTypeName(typeid(exception)),
      WithActualMessage(exception));
}

std::string MakeNonStdExceptionMessage(std::string_view statement) {
  return fmt::format(
      "'{}' throws a non-std::exception-derived exception. "
      "Such exceptions are not supported by utest. ",
      statement);
}

std::string MakeNotThrownExceptionMessage(std::string_view statement,
                                          const std::type_info& expected_type,
                                          std::string_view expected_message) {
  return fmt::format(
      "  Expected: '{}' throws '{}'{}.\n"
      "  Actual: it does not throw. ",
      statement, compiler::GetTypeName(expected_type),
      WithExpectedMessage(expected_message));
}

}  // namespace

std::string AssertThrow(std::function<void()> statement,
                        std::string_view statement_text,
                        std::function<bool(const std::exception&)> type_checker,
                        const std::type_info& expected_type,
                        std::string_view message_substring) {
  try {
    statement();
    return MakeNotThrownExceptionMessage(statement_text, expected_type,
                                         message_substring);
  } catch (const std::exception& ex) {
    const std::string_view message{ex.what()};
    if (!type_checker(ex)) {
      return MakeMismatchedExceptionTypeMessage(
          statement_text, ex, expected_type, message_substring);
    }
    if (message.find(message_substring) == std::string_view::npos) {
      return MakeMismatchedExceptionMessage(statement_text, ex, expected_type,
                                            message_substring);
    }
    return {};
  } catch (...) {
    return MakeNonStdExceptionMessage(statement_text);
    // don't rethrow to make sure a nice message is displayed
  }
}

std::string AssertNoThrow(std::function<void()> statement,
                          std::string_view statement_text) {
  try {
    statement();
    return {};
  } catch (const std::exception& ex) {
    return MakeThrownExceptionMessage(statement_text, ex);
  } catch (...) {
    return MakeNonStdExceptionMessage(statement_text);
    // don't rethrow to make sure a nice message is displayed
  }
}

}  // namespace utest::impl

USERVER_NAMESPACE_END
