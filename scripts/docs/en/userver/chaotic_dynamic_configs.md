## Chaotic dynamic config codegen

Dynamic configs are runtime-adjustable settings fetched from a config service.
Chaotic can generate the C++ type, parser, and variable wrapper for each config variable
from a YAML schema file, eliminating hand-written boilerplate.

See @ref scripts/docs/en/userver/dynamic_config.md for the broader dynamic config system.


### Input file format

Each variable lives in its own YAML file. The file contains a JSONSchema object with two
top-level keys:

* `schema` — the schema of the variable value (a full JSONSchema object).
* `default` — the default value used when the config service does not provide one.

`schema` may contain a nested `definitions` section for auxiliary types referenced by `$ref`.

Example `MY_FEATURE_FLAGS.yaml`:

```
# yaml
schema:
    type: object
    additionalProperties: false
    properties:
        enable_new_logic:
            type: boolean
        timeout_ms:
            type: integer
            format: int64
            minimum: 0
    required:
      - enable_new_logic
default:
    enable_new_logic: false
    timeout_ms: 5000
```


### CMake integration

Use `userver_target_generate_chaotic_dynamic_configs()` to generate a CMake library target
from a glob of variable files:

```cmake
userver_target_generate_chaotic_dynamic_configs(
    myservice-dynconf                           # CMake target name
    ${CMAKE_CURRENT_SOURCE_DIR}/dynconf/*.yaml  # glob for variable YAML files
)
target_link_libraries(myservice myservice-dynconf)
```

| Parameter | Description |
|---|---|
| First positional (`TARGET`) | CMake library target name to create. |
| Second positional (`SCHEMAS_REGEX`) | Shell glob pattern passed to `file(GLOB …)` to discover input YAML files. |

The macro links the target against `userver::core` and `userver::chaotic` automatically.


### Generated file layout

For each variable file `NAME.yaml` the following files are generated under
`${CMAKE_CURRENT_BINARY_DIR}/dynamic_configs/`:

| Generated file | Contents |
|---|---|
| `include/dynamic_config/variables/NAME.types_fwd.hpp` | Forward declarations of generated types |
| `include/dynamic_config/variables/NAME.types.hpp` | Type definitions |
| `include/dynamic_config/variables/NAME.types_parsers.ipp` | DOM / YAML parsers |
| `include/dynamic_config/variables/NAME.hpp` | Variable wrapper (`dynamic_config::Variable<T>`) |
| `src/dynamic_config/variables/NAME.types.cpp` | Type definitions source |
| `src/dynamic_config/variables/NAME.cpp` | Variable initializer (registers default value) |

Include `dynamic_config/variables/NAME.hpp` to use the variable in C++:

```cpp
#include <dynamic_config/variables/MY_FEATURE_FLAGS.hpp>

// In a handler or task:
auto cfg = config_source.GetSnapshot();
const auto& flags = cfg[dynamic_config::MY_FEATURE_FLAGS];
if (flags.enable_new_logic) { ... }
```


### Direct CLI reference

`chaotic-gen-dynamic-configs` is the underlying executable.

| Flag | Required | Description |
|---|---|---|
| `-o` / `--output-dir` | yes | Directory for generated files. |
| `file …` | yes | One or more YAML variable files. |
| `-I` / `--include-dir` | no (repeatable) | Extra include path for `x-usrv-cpp-type` header lookup. |
| `--clang-format` | no | clang-format binary; set to empty string to disable. |


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/chaotic.md | @ref scripts/docs/en/userver/chaotic_clients.md ⇨
@htmlonly </div> @endhtmlonly
