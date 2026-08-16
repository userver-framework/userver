## Chaotic handlers (server side)

Chaotic can generate C++ server-side handler base classes, request/response types, and
view stub files directly from an OpenAPI 3.x or Swagger 2.0 schema.
This eliminates hand-written boilerplate and keeps the server contract in sync with the schema.

For generating HTTP clients instead, see @ref scripts/docs/en/userver/chaotic_clients.md.


### What gets generated

For each HTTP operation in the schema chaotic produces:

* **Handler base class** — a userver `HttpHandlerBase` subclass with typed `Handle()` method.
* **Request / response types** — C++ structs with JSON parsers and serializers.
* **View stub** — a minimal `.hpp`/`.cpp` pair in `SRC_DIR` that you fill in with business logic.
  Existing view stubs are **never overwritten** on subsequent runs.
* **`config.chaotic.yaml`** — static config fragment for all generated handlers; merged into
  the service config by `userver_generate_config_yaml()`.


### Quickstart

**Step 1.** Write an OpenAPI schema (OpenAPI 3.x YAML or Swagger 2.0 JSON/YAML):

@include samples/chaotic_openapi_service/handlers/insecure/openapi.yaml

**Step 2.** Declare the handler target in `CMakeLists.txt`:

@snippet samples/chaotic_openapi_service/CMakeLists.txt chaotic-handler

**Step 3.** Implement the generated view stub. Chaotic writes a skeleton to
`src/handlers/NAME/OPERATION/view.cpp` on the first run:

@snippet samples/chaotic_openapi_service/src/handlers/insecure/insecuresecretpost/view.cpp view-impl

Subsequent runs leave `view.cpp` untouched.

**Step 4.** Register the handler list in `main.cpp`:

@snippet samples/chaotic_openapi_service/main.cpp register-handlers

**Step 5.** Merge the generated `config.chaotic.yaml` into the service config:

@snippet samples/chaotic_openapi_service/CMakeLists.txt generate-config


### CMake reference

#### `userver_target_generate_openapi_handlers(TARGET)`

| Parameter | Kind | Default | Description |
|---|---|---|---|
| First positional (`TARGET`) | — | **required** | CMake library target name to create. |
| `NAME` | one-value | **required** | Service name; used in generated include paths (`handlers/NAME/…`) and as the default C++ namespace base. |
| `SCHEMAS` | multi-value | **required** | OpenAPI YAML/JSON source files. |
| `OUTPUT_DIR` | one-value | `${CMAKE_CURRENT_BINARY_DIR}/NAME` | Destination for generated `include/` and `src/` trees. |
| `SRC_DIR` | one-value | — | If set, view stubs for new operations are written here under `handlers/NAME/OP/`. Omit to skip view-stub generation. |
| `ARGS` | multi-value | — | Extra arguments forwarded verbatim to `chaotic-openapi-gen`. |

The macro links the target against `userver::chaotic-openapi` and exposes the generated
`include/` directory. It also attaches the generated `config.chaotic.yaml` path to the target
via the `INTERFACE_USERVER_EXTRA_CONFIG_YAML_PATHS` property, which
`userver_generate_config_yaml()` collects transitively.

#### `userver_generate_config_yaml(BINARY_TARGET)`

Merges `config.chaotic.yaml` files collected from all handler targets in the link graph of
`BINARY_TARGET` with the caller-provided base configs, producing a single `config.yaml`.

| Parameter | Kind | Default | Description |
|---|---|---|---|
| First positional (`BINARY_TARGET`) | — | **required** | Executable or library whose transitive link graph is scanned for `config.chaotic.yaml` files. |
| `OUTPUT` | one-value | **required** | Path for the final merged `config.yaml`. |
| `BASE_CONFIGS` | multi-value | — | User-provided config fragments applied last (highest priority). |

The macro creates a `${BINARY_TARGET}_config` target that depends on the output file.


### Generated file layout

For a service named `greeter` with an operation `GET /hello` (operationId `getHello`), the
layout under `OUTPUT_DIR` is:

```
include/handlers/greeter/
    get_hello/
        handler.hpp      # handler base class
        requests.hpp     # request types
        responses.hpp    # response types
    openapi.hpp          # shared schema types
src/handlers/greeter/
    get_hello/
        handler.cpp
        requests.cpp
        responses.cpp
    openapi.cpp
config.chaotic.yaml      # static config fragment
```

View stubs (written to `SRC_DIR/handlers/greeter/get_hello/`):

```
view.hpp    # view interface declaration
view.cpp    # view stub — implement this
```


### Direct CLI reference

`chaotic-openapi-gen` is the underlying executable. Handler-relevant flags:

| Flag | Required | Description |
|---|---|---|
| `files …` | yes | One or more OpenAPI/Swagger YAML/JSON files. |
| `--name` | yes | Service name (sets include-path prefix and default namespace). |
| `--gen` | yes | `handlers` — handler classes only; `handlers+views` — handlers and view stubs; `views` — view stubs only. |
| `-o` / `--output-dir` | for `handlers`/`handlers+views` | Destination for generated `include/` and `src/`. |
| `--src-dir` | for `views`/`handlers+views` | Directory where view stubs are written. |
| `--namespace` | no | Override the C++ namespace (default: `handlers::NAME`). |
| `-I` / `--include-dir` | no (repeatable) | Extra include path for `x-usrv-cpp-type` header lookup. |
| `--clang-format` | no | clang-format binary; set to empty string to disable. |
| `-u` / `--userver` | no | userver namespace macro name (default: `USERVER_NAMESPACE`). |


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/chaotic_clients.md | @ref scripts/docs/en/userver/sql_files.md ⇨
@htmlonly </div> @endhtmlonly
