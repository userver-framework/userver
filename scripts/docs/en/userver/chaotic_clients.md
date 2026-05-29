# Chaotic clients

Chaotic is able to generate a HTTP client from an OpenAPI schema.
You declare the API of the endpoint in [OpenAPI 3.x format](https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.1.1.md)
(Swagger 2.0 is also accepted)
and chaotic generates parsers, serializers and a client for you.

For generating server-side handlers instead, see @ref scripts/docs/en/userver/chaotic_handlers.md.


# Quickstart

First, define OpenAPI schema in one or multiple yaml files:

@include samples/chaotic_openapi_service/clients/test.yaml

Second, declare the client in `CMakeLists.txt`:

@snippet samples/chaotic_openapi_service/CMakeLists.txt chaotic

`userver_target_generate_openapi_client()` parameters:

| Parameter | Kind | Default | Description |
|---|---|---|---|
| First positional (`TARGET`) | — | **required** | CMake library target name to create. |
| `NAME` | one-value | **required** | Client name; used in generated include paths (`clients/NAME/…`) and as the default C++ namespace base. |
| `SCHEMAS` | multi-value | **required** | OpenAPI YAML/JSON source files. |
| `OUTPUT_DIR` | one-value | `${CMAKE_CURRENT_BINARY_DIR}/NAME` | Where generated `include/` and `src/` trees are placed. |
| `ARGS` | multi-value | — | Extra arguments forwarded verbatim to `chaotic-openapi-gen` (e.g. `--dynamic-config CONFIG_KEY`). |

Third, register the client component in the component system:

@snippet samples/chaotic_openapi_service/main.cpp register-client

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

```
# yaml
    test-client:
      middlewares:
        logging:
          request_level: info  # log level to log request body
          response_level: warning  # log level to log response body
          body_limit: 10000  # trim body to max size
```

## Dynamic Quality-of-service configs (QOS)

Clients may fetch attempts and retries from dynamic config.
First, pass the config key name to the generator via `ARGS "--dynamic-config" "MY_CLIENT_QOS"`
in `CMakeLists.txt`. This causes chaotic to emit a `kQosConfig` constant in the generated
`clients::my_client` namespace.

Use `qos-{client_name}` middleware in static config (change `test-client` to your client name,
and `qos-test` to `qos-{client_name}`):

@snippet samples/chaotic_openapi_service/static_config.user.yaml qos-config

Register `QosMiddlewareFactory` in `main.cpp` before the client component:

@snippet samples/chaotic_openapi_service/main.cpp register-qos

## Proxy

HTTP proxy may be enabled using `proxy` middleware in static config:

```
# yaml
    test-client:
      middlewares:
        proxy:
          url: my-proxy.org
```

## HTTP Redirects

By default a client stops at the first redirect and interprets it as a response.
If you want to follow redirects, use `follow-redirects` middleware:

```
# yaml
    test-client:
      middlewares:
        follow-redirects: {}
```


# Direct CLI reference

`chaotic-openapi-gen` is the underlying executable invoked by `userver_target_generate_openapi_client()`.

| Flag | Required | Description |
|---|---|---|
| `files …` | yes | One or more OpenAPI/Swagger YAML/JSON files. |
| `--name` | yes | Client or service name (sets include-path prefix and default namespace). |
| `--gen` | yes | What to generate: `client`, `handlers`, `views`, or `handlers+views`. |
| `-o` / `--output-dir` | for `client`/`handlers`/`handlers+views` | Destination for generated `include/` and `src/` trees. |
| `--src-dir` | for `views`/`handlers+views` | Directory where view stub files are written. |
| `--namespace` | no | Override the C++ namespace (default: `clients::NAME` for client, `handlers::NAME` for handlers). |
| `--dynamic-config` | no | Dynamic config key name for QOS; emits a `kQosConfig` constant. |
| `-I` / `--include-dir` | no (repeatable) | Extra include path for `x-usrv-cpp-type` header lookup. |
| `--clang-format` | no | clang-format binary; set to empty string to disable. |
| `-u` / `--userver` | no | userver namespace macro name (default: `USERVER_NAMESPACE`). |


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/chaotic_dynamic_configs.md | @ref scripts/docs/en/userver/chaotic_handlers.md ⇨
@htmlonly </div> @endhtmlonly
