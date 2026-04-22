# RichStatus

@ref ugrpc::RichStatus is a userver wrapper around `google::rpc::Status` that provides a convenient API for creating and managing
gRPC status responses with rich error details. It follows the [Google AIP-193](https://google.aip.dev/193) standard for structured error reporting,
allowing services to provide detailed error information that clients can use for better error handling and user experience.

## Introduction to RichStatus

@ref ugrpc::RichStatus allows you to create gRPC status responses with structured error information conforming to the Google RPC
error model. It supports adding multiple error details of various types to provide comprehensive error information to clients.

The main benefits of using @ref ugrpc::RichStatus include:
- Structured error information that can be programmatically processed by clients
- Support for multiple error detail types in a single response
- Easy conversion to and from standard gRPC status objects
- Compliance with Google AIP-193 standard for error details

## Creating RichStatus with Error Details

You can create a @ref ugrpc::RichStatus in several ways:

1. Default constructor creates an OK status with no error details:

   @snippet grpc/tests/rich_status_test.cpp rich_status_default_constructor

2. From a gRPC status:

   @snippet grpc/tests/rich_status_test.cpp rich_status_from_grpc_status

3. With status code, message, and error details:

   @snippet grpc/tests/rich_status_test.cpp rich_status_constructor_with_details

4. Adding details after creation:

   @snippet grpc/tests/rich_status_test.cpp rich_status_add_detail_chaining

## Supported Error Detail Types

@ref ugrpc::RichStatus supports all standard Google error detail types:

- @ref ugrpc::ErrorInfo - Provides structured error information about the cause of an error
- @ref ugrpc::RetryInfo - Describes when the client can retry a failed request
- @ref ugrpc::DebugInfo - Provides debugging information such as stack traces
- @ref ugrpc::QuotaFailure - Describes quota violations that caused the request to fail
- @ref ugrpc::PreconditionFailure - Describes preconditions that failed during request processing
- @ref ugrpc::BadRequest - Describes violations in a client request
- @ref ugrpc::RequestInfo - Contains metadata about the request for debugging and logging
- @ref ugrpc::ResourceInfo - Provides information about a resource that is related to the error
- @ref ugrpc::Help - Provides links to documentation and help resources
- @ref ugrpc::LocalizedMessage - Provides a localized error message that is safe to return to the user

## Extracting Error Details on the Client Side

When a client receives an error response with @ref ugrpc::RichStatus details, it can extract specific error details using the `TryGetDetail` method:

@snippet grpc/tests/detailed_error_test.cpp try_get_rich_error_detail

## Integration with gRPC Services

In a gRPC service implementation, you can return a @ref ugrpc::RichStatus by converting it to a gRPC status:

@snippet grpc/tests/detailed_error_test.cpp rich_status_usage

The @ref ugrpc::RichStatus is automatically converted to a gRPC status with serialized error details that can be unpacked by the client.

## Examples

### ErrorInfo Example

@snippet grpc/tests/rich_status_test.cpp error_info_example

### RetryInfo Example

@snippet grpc/tests/rich_status_test.cpp retry_info_example

### BadRequest Example

@snippet grpc/tests/rich_status_test.cpp bad_request_example

### LocalizedMessage Example

@snippet grpc/tests/rich_status_test.cpp localized_message_example

For more information about the Google error model, see:
- [Google AIP-193](https://google.aip.dev/193)
- [gRPC Error Model](https://grpc.io/docs/guides/error/)
