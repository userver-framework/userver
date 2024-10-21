#pragma once

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/ugrpc/client/client_qos.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

/// [qos config key]
inline const dynamic_config::Key<ugrpc::client::ClientQos> kUnitTestClientQos{
    "UNIT_TEST_CLIENT_QOS",
    dynamic_config::DefaultAsJsonString{
        R"(
          {
            "__default__": {
              "timeout-ms": 100
            }
          }
        )",
    },
};
/// [qos config key]

}  // namespace tests

USERVER_NAMESPACE_END
