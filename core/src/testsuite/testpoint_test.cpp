#include <userver/testsuite/testpoint.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/testsuite/testpoint_control.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class EchoTestpointClient final : public testsuite::TestpointClientBase {
 public:
  EchoTestpointClient() = default;

  ~EchoTestpointClient() override { Unregister(); }

  void Execute(const std::string& name, const formats::json::Value& json,
               const Callback& callback) const override {
    if (!callback) return;
    callback(formats::json::MakeObject("name", name, "body", json));
  }
};

bool TryExecuteTestpoint() {
  bool called = false;
  TESTPOINT_CALLBACK("name", {}, [&](const auto&) { called = true; });
  return called;
}

}  // namespace

UTEST(Testpoint, Smoke) {
  testsuite::TestpointControl testpoint_control;
  EchoTestpointClient testpoint_client;
  testpoint_control.SetClient(testpoint_client);
  testpoint_control.SetAllEnabled();

  formats::json::Value response;
  TESTPOINT_CALLBACK(
      "what", formats::json::ValueBuilder{"foo"}.ExtractValue(),
      [&](const formats::json::Value& json) { response = json; });
  EXPECT_EQ(response, formats::json::MakeObject("name", "what", "body", "foo"));
}

namespace {}  // namespace

UTEST(Testpoint, ReentrableInitialization) {
  for (int i = 0; i < 2; ++i) {
    testsuite::TestpointControl testpoint_control;

    {
      EchoTestpointClient testpoint_client;
      testpoint_control.SetClient(testpoint_client);
      testpoint_control.SetAllEnabled();

      EXPECT_TRUE(TryExecuteTestpoint());
    }

    EXPECT_FALSE(TryExecuteTestpoint());
  }

  EXPECT_FALSE(TryExecuteTestpoint());
}

UTEST(Testpoint, MultipleTestpointsInSameScope) {
  testsuite::TestpointControl testpoint_control;
  EchoTestpointClient testpoint_client;
  testpoint_control.SetClient(testpoint_client);
  testpoint_control.SetAllEnabled();

  int times_called = 0;
  TESTPOINT("name", [&] {
    ++times_called;
    return formats::json::Value{};
  }());
  TESTPOINT("name", [&] {
    ++times_called;
    return formats::json::Value{};
  }());
  TESTPOINT_CALLBACK("name", {}, [&](const auto&) { ++times_called; });
  TESTPOINT_CALLBACK("name", {}, [&](const auto&) { ++times_called; });
  EXPECT_EQ(times_called, 4);
}

USERVER_NAMESPACE_END
