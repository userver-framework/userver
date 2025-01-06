# S3 client tutorial

## Before you start

Take a look at official S3 documentation. Have your access key and secret key ready.

## Step by step guide

In this example, we will create a client and make a request to S3 endpoint.

### Installation


Find and link to userver module:

@snippet samples/s3api/CMakeLists.txt  s3api

## Creating client in code

We recommend wrapping your clients into some component. This way you can make sure that reference to http_client
outlives S3 client. It can look, for example, like this:

@snippet samples/s3api/src/s3api_client.hpp s3_sample_component


To create client itself, you do only few simple steps:

@snippet samples/s3api/src/s3api_client.cpp create_client

## Using client

Here is usage example

@snippet samples/s3api/src/s3api_client.cpp using_client

## Testing

We provide google mock for client, that you can access by including header
```cpp
#include <userver/s3api/utest/client_gmock.hpp>
```

With this mock, you have full power of Google Mock at your fingertips:

@snippet samples/s3api/unittests/client_test.cpp  client_tests

To add tests to your project, add appropriate lines to CMakeLists.txt, like this:

@snippet samples/s3api/CMakeLists.txt  gtest


## Testsuite support

Testsuite module is provided as part of testsuite plugins and can be found at:

@ref testsuite/pytest_plugins/pytest_userver/plugins/s3api.py


## Full Sources

See the full example at:

* @ref samples/s3api/src/s3api_client.hpp
* @ref samples/s3api/src/s3api_client.cpp
* @ref samples/s3api/unittests/client_test.cpp
* @ref samples/s3api/main.cpp
* @ref samples/s3api/CMakeLists.txt


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/multipart_service.md | @ref scripts/docs/en/userver/tutorial/json_to_yaml.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/s3api/src/s3api_client.hpp
@example samples/s3api/src/s3api_client.cpp
@example samples/s3api/unittests/client_test.cpp
@example samples/s3api/main.cpp
@example samples/s3api/CMakeLists.txt

@example testsuite/pytest_plugins/pytest_userver/plugins/s3api.py
