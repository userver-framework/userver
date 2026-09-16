## SQS JSON Client

**Quality:** @ref QUALITY_TIERS "Silver Tier".

🐙 **userver** provides @ref sqs::JsonClient, an Amazon SQS-compatible client
for the AWS JSON protocol.

The client uses AWS SDK SQS request/result types (`Aws::SQS::Model::*`) and
sends HTTP through @ref clients::http::Client. It does not inherit
`Aws::SQS::SQSClient` and does not use the AWS HTTP client or AWS thread-pool
async API.

```cpp
#include <userver/sqs/json_client.hpp>

#include <aws/sqs/model/SendMessageRequest.h>
```

## Build

Enable the library when building userver (`USERVER_FEATURE_SQS`, off by default).
See @ref scripts/docs/en/userver/build/options.md.

In a service:

```cmake
find_package(userver COMPONENTS core sqs REQUIRED)
target_link_libraries(${PROJECT_NAME} userver::sqs)
```

The library depends on AWS SDK `core`/`sqs` and `userver::core`.

## Lifetime

`Aws::InitAPI` is not required: the client uses AWS request/result types as DTOs
and signs requests with userver crypto. `JsonClient` does not own the HTTP
client; `clients::http::Client` must outlive it.

Call SQS methods from a userver coroutine: they perform HTTP via
`clients::http::Request::perform()`.

@snippet json_client_test.cpp  Sample SQS JsonClient

## Configuration

`JsonClient` takes `clients::http::Client&`, @ref sqs::Credentials and
@ref sqs::ClientSettings.

`endpoint`
: SQS-compatible JSON API URL. Required.

`region`
: Signing region for SigV4. Required.

`timeout`
: Whole HTTP request timeout, including long polling on `ReceiveMessage`.

`verify_ssl`
: TLS certificate verification for the userver HTTP client.

If credentials are non-empty, requests are signed with AWS SigV4 using userver
crypto. If they are empty, the request is sent unsigned.

## Example

@snippet json_client_test.cpp  Sample SQS send and receive

## Supported Operations

The client currently implements the JSON protocol for:

`AddPermission`, `ChangeMessageVisibility`, `ChangeMessageVisibilityBatch`,
`CreateQueue`, `DeleteMessage`, `DeleteMessageBatch`, `DeleteQueue`,
`GetQueueAttributes`, `GetQueueUrl`, `ListDeadLetterSourceQueues`,
`ListQueueTags`, `ListQueues`, `PurgeQueue`, `ReceiveMessage`,
`RemovePermission`, `SendMessage`, `SendMessageBatch`, `SetQueueAttributes`,
`TagQueue`, `UntagQueue`.

See also @ref sqs::JsonClient.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/libraries/s3api.md | @ref scripts/docs/en/userver/libraries/grpc-reflection.md ⇨
@htmlonly </div> @endhtmlonly
