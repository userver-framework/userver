#include <userver/testsuite/testpoint.hpp>

#include <thread>

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

  void Execute(std::string_view name, const formats::json::Value& json,
               Callback callback) const override {
    callback(formats::json::MakeObject("name", name, "body", json));
  }
};

struct Exception final : std::exception {};

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

UTEST(Testpoint, Exceptions) {
  testsuite::TestpointControl testpoint_control;
  EchoTestpointClient testpoint_client;
  testpoint_control.SetClient(testpoint_client);
  testpoint_control.SetAllEnabled();

  try {
    TESTPOINT_CALLBACK("name-throws", {}, [](const auto&) {
      // Callbacks may throw. It is fine and used in SQL drivers of userver for
      // error injection.
      throw Exception();
      return formats::json::Value{};
    });
    FAIL() << "Exception was swallowed";
  } catch (const Exception&) {
  }
}

UTEST_MT(Testpoint, ExceptionsNoncoro, 2) {
  testsuite::TestpointControl testpoint_control;
  EchoTestpointClient testpoint_client;
  testpoint_control.SetClient(testpoint_client);
  testpoint_control.SetAllEnabled();

  auto& tp = engine::current_task::GetTaskProcessor();

  std::thread([&] {
    try {
      TESTPOINT_CALLBACK_NONCORO("name-throws-noncoro", {}, tp,
                                 [](const auto&) {
                                   // Callbacks may throw. It is fine and used
                                   // in SQL drivers of userver for error
                                   // injection.
                                   throw Exception();
                                   return formats::json::Value{};
                                 });
      FAIL() << "Exception was swallowed";
    } catch (const Exception&) {
    }
  }).join();
}

UTEST(Testpoint, Inactive) {
  testsuite::TestpointControl testpoint_control;
  EchoTestpointClient testpoint_client;
  testpoint_control.SetClient(testpoint_client);

  try {
    TESTPOINT("name", true ? throw Exception() : formats::json::Value{});
  } catch (const Exception&) {
    FAIL() << "Exception was thrown on inactive testpoint. JSON expression was "
              "evaluated!";
  }

  try {
    TESTPOINT_CALLBACK("name2", {}, [](const auto&) {
      // Callbacks must not be executed.
      throw Exception();
      return formats::json::Value{};
    });
  } catch (const Exception&) {
    FAIL() << "Exception was thrown on inactive testpoint. Callback was called";
  }
}

TEST(Testpoint, InactiveNoncoro) {
  try {
    TESTPOINT_NONCORO("name-noncoro",
                      true ? throw Exception() : formats::json::Value{},
                      engine::current_task::GetTaskProcessor());
  } catch (const Exception&) {
    FAIL() << "Exception was thrown on inactive testpoint. JSON expression was "
              "evaluated!";
  }

  try {
    TESTPOINT_CALLBACK_NONCORO("name2-noncoro", {},
                               engine::current_task::GetTaskProcessor(),
                               [](const auto&) {
                                 // Callbacks must not be executed.
                                 throw Exception();
                                 return formats::json::Value{};
                               });
  } catch (const Exception&) {
    FAIL() << "Exception was thrown on inactive testpoint. Callback was called";
  }
}

USERVER_NAMESPACE_END
