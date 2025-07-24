# Chaotic clients

Chaotic is able to generate a http client from OpenAPI schema.
You declare API of the endpoint in [OpenAPI format](https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.1.1.md)
and chaotic generates parsers, serializers and a client for you.


# Quickstart

First, define OpenAPI schema in one or multiple yaml files.

```yaml
openapi: 3.0.0
...
```
Second, declare the client in `CMakeLists.txt`:
@snippet samples/chaotic_openapi_service/CMakeLists.txt chaotic

Third, register the client component in the component system:

```cpp
int main(int argc, char* argv[]) {
    ...
      .Append<::clients::test::Component>();
```

Fourth, get a reference to the client...

@snippet samples/chaotic_openapi_service/src/hello_handler.cpp get

...and use it:

@snippet samples/chaotic_openapi_service/src/hello_handler.cpp use


# Testing

If you test a service with testsuite, you mock the client.

Change client URL in `conftest.py`:

@snippet samples/chaotic_openapi_service/testsuite/conftest.py URL

Now you may mock the client with mockserver:

@snippet samples/chaotic_openapi_service/testsuite/test_hello.py Functional test


# Extending the client

The client logic may be extended with middlewares.
Middleware's code can be executed before the request is sent to the server and after it is processed.

## Logging

If you want to log every in/out client body, use `logging` middleware in static config:

```yaml
    test-client:
      middlewares:
        logging:
          request_level: info  # log level to log request body
          response_level: warning  # log level to log response body
          body_limit: 10000  # trim body to max size
```

## Dynamic Quality-of-service configs (QOS)

Clients may fetch attempts and retries from dynamic config.
Use `qos-{client_name}` middleware in static config (change "test-client" to your client name):

```yaml
    test-client:
      middlewares:
        qos-test-client: {}
```

And register it in `main.cpp`:

```cpp
auto component_list =
  userver::components::ComponentList()
  .Append<userver::chaotic::openapi::QosMiddleware<clients::test_client::kQosConfig>>("chaotic-client-middleware-qos-test-client")
```

## Proxy

HTTP proxy may be enabled using `proxy` middleware in static config:

```yaml
    test-client:
      middlewares:
        proxy:
          url: my-proxy.org
```

## HTTP Redirects

By default a client stops at the first redirect and interprets it as a response.
If you want to follow redirects, use `follow-redirects` middleware:

```yaml
    test-client:
      middlewares:
        follow-redirects: {}
```


