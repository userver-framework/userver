#include <userver/utest/using_namespace_userver.hpp>

/// [Sample TESTPOINT_CALLBACK usage cpp]
#include <userver/testsuite/testpoint.hpp>

void SomeMethod(/*...*/) {
  using formats::json::Value;
  // ...
  TESTPOINT_CALLBACK("mytp", Value(), [](const Value& doc) {
    if (doc.IsObject()) {
      if (doc["inject_failure"].As<bool>())
        throw std::runtime_error("injected error");
    }
  });
  // ...
}
/// [Sample TESTPOINT_CALLBACK usage cpp]

/*
/// [Sample TESTPOINT_CALLBACK usage python]

// Somewhere in Python in Testsuite:
async def test_some_method(myservice, testpoint, ...):
    @testpoint('mytp')
    def mytp(data):
        return {'inject_failure': True}

    response = await myservice.get('/somemethod')
    assert response.status_code == 500

    # testpoint supports callqueue interface
    assert mytp.times_called == 1
/// [Sample TESTPOINT_CALLBACK usage python]
*/
