#pragma once

/// @file userver/testsuite/testpoint_control.hpp
/// @brief @copybrief testsuite::TestpointControl

#include <functional>
#include <string>
#include <unordered_set>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

/// @brief Base testpoint client. Used to report TESTPOINT executions to
/// testsuite.
///
/// Do not use directly unless you are writing a component for handling
/// testpoints.
class TestpointClientBase {
 public:
  using Callback = std::function<void(const formats::json::Value&)>;

  virtual ~TestpointClientBase();

  /// @param name the name of the testpoint
  /// @param json the request that will be passed to testsuite handler
  /// @param callback will be invoked with the response if the testpoint has
  /// been handled on the testsuite side successfully
  virtual void Execute(const std::string& name,
                       const formats::json::Value& json,
                       const Callback& callback) const = 0;

 protected:
  /// Must be called in destructors of derived classes
  void Unregister() noexcept;
};

/// @brief Testpoint control interface for testsuite
///
/// All methods are coro-safe.
/// All testpoints are disabled by default.
/// Only 1 TestpointControl instance may exist globally at a time.
class TestpointControl final {
 public:
  TestpointControl();
  ~TestpointControl();

  /// @brief Enable only the selected testpoints
  void SetEnabledNames(std::unordered_set<std::string> names);

  /// @brief Enable all defined testpoints
  ///
  /// Testpoints, for which there is no registered handler on the testsuite
  /// side, will still be skipped, at the cost of an extra request.
  void SetAllEnabled();

  /// @brief Makes a testpoint client globally accessible from testpoints. It
  /// will unregister itself on destruction.
  /// @note Only 1 client may be registered at a time.
  void SetClient(TestpointClientBase& client);
};

}  // namespace testsuite

USERVER_NAMESPACE_END
