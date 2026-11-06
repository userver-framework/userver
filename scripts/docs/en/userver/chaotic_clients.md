# Chaotic clients

Chaotic is able to generate an HTTP client from an OpenAPI schema.
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

Generated client components also register their public `Client&` in the
Chaotic handler dependency repository. If the service does not add a generated
`ChaoticHandlersList()`, register the repository explicitly in `main.cpp`:

```cpp
component_list.Append<USERVER_NAMESPACE::components::Container<
    USERVER_NAMESPACE::chaotic::openapi::server::dependencies::Factories>>();
```

A generated handler can obtain the client through the generated dependency
tag:

```cpp
auto& client = deps[clients::test::kDependency];
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


# Authentication

## HTTP Digest Authentication

If an operation in the OpenAPI schema is annotated with a `securityScheme` of type `http` and scheme `digest`,
the generated client will automatically apply HTTP Digest authentication for that operation.

### OpenAPI schema

Declare the security scheme in `components.securitySchemes` and reference it on the operation:

@snippet samples/chaotic_openapi_service/clients/secure.yaml security scheme

@snippet samples/chaotic_openapi_service/clients/secure.yaml security requirement

Generate and register the client in the same way as an unauthenticated client:

@snippet samples/chaotic_openapi_service/CMakeLists.txt digest-client-cmake

@snippet samples/chaotic_openapi_service/main.cpp register-digest-client

### Credentials in secdist

Credentials are loaded from the `Secdist` component at service startup.
The required JSON layout is:

```json
{
  "http_digest": {
    "<client-name>": {
      "<scheme-name>": { "username": "alice", "password": "secret" }
    }
  }
}
```

`<scheme-name>` is the `securitySchemes` key, `<client-name>` is the `NAME` of
`userver_target_generate_openapi_client()`. A missing entry is reported at service start
with the exact key that was looked up.

A runnable example is part of `chaotic_openapi_service`:

@include samples/chaotic_openapi_service/secure_data.json

Configure the client URL as usual; the testsuite may redirect it to mockserver:

@snippet samples/chaotic_openapi_service/static_config.user.yaml digest-client-config

@snippet samples/chaotic_openapi_service/testsuite/conftest.py digest-client-url

No credentials are passed at the call site—the generated client performs the challenge/response exchange:

@snippet samples/chaotic_openapi_service/src/handlers/insecure/callsecretget/view.cpp client-use

## Other security schemes

HTTP Digest is the only scheme the generated client applies itself. An operation secured by any other scheme
(`http` with a scheme other than `digest`, `apiKey`, `oauth2` or `openIdConnect`) is generated as well, but sends
no credentials; the generated method's documentation names the scheme. Supply the credentials with a client
middleware, see `chaotic::openapi::client::Middleware`.

An operation must use at most one security requirement with at most one scheme in it; alternative (OR) and combined
(AND) requirements are rejected. Scopes named by the requirement do not change the generated client: the credentials
the caller supplies either grant them or not.


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


# Expected HTTP status codes in client spans

Use `x-usrv-expected-status-codes` in an OpenAPI 3.x or Swagger 2.0 schema
to mark explicitly expected HTTP responses:

```yaml
openapi: 3.0.0
info:
  title: Catalog
  version: '1.0'
x-usrv-expected-status-codes: [404]
paths:
  /items:
    get:
      x-usrv-expected-status-codes: [409]
      responses:
        '404':
          description: Unexpected missing item
        '409':
          description: Expected conflict
  /required-items:
    get:
      x-usrv-expected-status-codes: []
      responses:
        '404':
          description: Unexpected missing item
```

The root extension supplies the client default for operations in that schema
document. An operation's list replaces it. Listing a response in `responses`
does not make its status expected. Codes must be integers in the range 100–599.

For one generated-client call, set
`chaotic::openapi::client::CommandControl::expected_status_codes`.
Priority is `per-call > operation > client`; lists are replaced, not merged.
An unset optional inherits the schema setting.

@snippet chaotic-openapi/integration_tests/src/expected_status_codes_test.cpp expected-status-codes-per-call

Pass `cc` as the generated method's last argument. An explicitly empty set clears inherited codes:

@snippet chaotic-openapi/integration_tests/src/expected_status_codes_test.cpp expected-status-codes-clear

Set the optional to `std::nullopt` to restore inheritance:

@snippet chaotic-openapi/integration_tests/src/expected_status_codes_test.cpp expected-status-codes-inherit

The set is self-contained, uses no dynamic memory,
supports `constexpr` and dynamic modification via `Set(status, expected)`.
Existing `constexpr` aggregate initialization of command control remains supported.

For an HTTP client without code generation, set expected codes on the request before sending it:

@snippet core/src/clients/http/tracing_manager_test.cpp http-client-expected-status-codes

An expected response omits the outgoing span's `error` tag.
@ref clients::http::Response::IsError returns false for it.
The retry-budget middleware accounts a completed expected response as successful.
Without any expected codes configured, HTTP statuses >=400 are classified as errors by default.

The following remain unchanged:

- the real HTTP status code;
- @ref clients::http::Response::raise_for_status and generated exceptions;
- response parsing;
- the HTTP retry-selection policy;
- HTTP-code metrics.

An expected 5xx can still be retried.
Each retry still consumes retry budget.
Transport errors and timeouts remain errors regardless of the configured codes.

Log consumers must respect the `error` field in the span. Setting expected
status codes alone does not change a consumer that derives span status from
the HTTP code.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/chaotic_dynamic_configs.md | @ref scripts/docs/en/userver/chaotic_handlers.md ⇨
@htmlonly </div> @endhtmlonly
