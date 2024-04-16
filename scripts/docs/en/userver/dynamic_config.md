## Dynamic config

For schemas of dynamic configs used by userver itself:
@see @ref scripts/docs/en/schemas/dynamic_configs.md

For information on how to write a service that distributes dynamic configs:
@see @ref scripts/docs/en/userver/tutorial/config_service.md

Dynamic config is a system of options that can be changed at runtime without
restarting the service.

Dynamic config is distributed as a large JSON object where each direct member
is called a dynamic config variable, or (somewhat confusingly) a dynamic config.
For example:

```json
{
  // ...
  "POSTGRES_DEFAULT_COMMAND_CONTROL": {
    "network_timeout_ms": 750,
    "statement_timeout_ms": 500
  },
  "USERVER_DEADLINE_PROPAGATION_ENABLED": true,
  // ...
}
```


### Why your service needs dynamic configs

Dynamic configs are an essential part of a reliable service with high
availability. The configs could be used as an emergency switch for new
functionality, selector for experiments, limits/timeouts/log-level setup,
proxy setup and so forth.

See @ref scripts/docs/en/userver/tutorial/production_service.md setup example.


### Adding and using your own dynamic configs

Dynamic config values are obtained via the dynamic_config::Source client
that is retrieved from components::DynamicConfig:

@snippet components/component_sample_test.cpp  Sample user component source

To read the config, you first need to define a global dynamic_config::Key
variable for it. Second, you should get the current value from the config
using the `Key`:

@snippet components/component_sample_test.cpp  Sample user component runtime config source

You can also subscribe to dynamic config updates using
dynamic_config::Source::UpdateAndListen functions, see their docs for details.

@anchor dynamic_config_key
#### What is needed to define a dynamic config

A dynamic_config::Key requires 3 main elements:

1. The config name.
    * We use UPPER_CASE for config names.

2. The way to parse the JSON value into a C++ object.
    * The default way that should be used in most scenarios is the normal
      JSON formats parsing.
      @see @ref dynamic_config_parsing
    * For edge case scenarios, dynamic_config::Key constructors provide various
      other ways to parse configs, refer to its documentation for details.

3. The default config value.
    * If the whole config type is
      a @ref dynamic_config_parsing_trivial "trivial type" , then the default
      value may be provided directly as a C++ literal.
    * For more complex types (e.g. objects), dynamic_config::DefaultAsJsonString
      should be used.
    * @see @ref dynamic_config_defaults "how defaults are used".


### Summary of main dynamic config classes

#### dynamic_config::Snapshot

A type-safe map that stores a snapshot of all used configs.
dynamic_config::Snapshot is cheaply copyable (and even more cheaply movable),
has the semantics of `std::shared_ptr<const Impl>`. An obtained
dynamic_config::Snapshot instance should be stored while its configs are used
or referred to.

#### dynamic_config::Source

A reference to the configs storage, required for obtaining the actual config
value and subscription to new values.

#### components::DynamicConfig

The component that stores configs in production and
@ref dynamic_config_testsuite "testsuite tests".

For unit tests, dynamic_config::StorageMock is used instead.

@see @ref dynamic_config_unit_tests


### Recommendations on working with dynamic config

1. Don't store and pass around `components::DynamicConfig&`. Most code should
   immediately call `GetSource()` on it, then use or store
   the dynamic_config::Source in a field.

2. Don't use dynamic_config::Source::UpdateAndListen to take a piece of config
   in `OnConfigUpdate` and store it into a "comfy" class field. It creates
   a synchronization problem out of the blue, while it has already been solved
   in the dynamic config API. Just store dynamic_config::Source in a field and
   call dynamic_config::Source::GetSnapshot where you need to read the config.

3. When using dynamic_config::Source::UpdateAndListen, be careful
   with the lifetime of the subscription handle
   (concurrent::AsyncEventSubscriberScope):

    * Put the subscription in the class, which method is specified
      as the callback, as the last field, or at least below all the fields that
      the callback needs to work. On class destruction, first the subscription
      object will be destroyed, guaranteeing that `OnConfigUpdate` will not be
      called afterward; then other fields of the class will be destroyed.

    * If the class has a non-default destructor (or might have in the future),
      call concurrent::AsyncEventSubscriberScope::Unsubscribe somewhere
      in the beginning of the destructor to ensure that `OnConfigUpdate` will
      not be called at the point when it is already unable to run safely.


@anchor dynamic_config_parsing

### Parsing dynamic configs

The following can be applied to parsing formats::json::Value in any context,
but it frequently comes up in the context of defining configs.

If your dynamic config is any more complex than
a @ref dynamic_config_parsing_trivial "trivial type", then you need to ensure
that JSON parsing is defined for it.

@anchor dynamic_config_parsing_trivial
#### Trivial types

JSON leafs can be parsed out of the box:

| OpenAPI   | C++ type                                 |
|-----------|------------------------------------------|
| `boolean` | `bool`                                   |
| `integer` | integer types, e.g. `uint32_t`, `size_t` |
| `numeric` | `double`                                 |
| `string`  | `std::string`                            |

If the whole config variable is of such type, then the default value for it can
be passed to dynamic_config::Key directly
(without using dynamic_config::DefaultAsJsonString).

#### Durations

Chrono durations are stored in JSON as integers:

* `json.As<std::chrono::seconds>()` will parse `10` as 10 seconds;
* `json.As<std::chrono::milliseconds>()` will parse `10` as 10 milliseconds;
* same for `minutes` and `hours`.

To allow humans to more easily differentiate between them while looking
at the JSON, we recommend ending the config name or the object label with one
of the following suffixes: `_MS`, `_SECONDS`, `_MINUTES`, `_HOURS`
(in the appropriate capitalization).

#### Enums

A string that may only be selected a finite range of values should be mapped
to C++ `enum class`. Parsers for enums currently have to be defined manually.
Example enum parser:

@snippet engine/task/task_processor_config.cpp  sample enum parser

#### Structs

Objects are represented as C++ structs. Parsers for structs currently
have to be defined manually. Example struct config:

@snippet dynamic_config/config_test.cpp  struct config hpp
@snippet dynamic_config/config_test.cpp  struct config cpp

#### Optionals

To represent optional (non-required) properties, use `std::optional` or `As`
with default. For example:

* `json.As<std::optional<int>>()` will parse a missing value
  as an empty optional;
* `json.As<int>(42)` will parse a missing value as `42`;
* `json.As<std::vector<int>>({})` will parse a missing value
  as a default-constructed `vector`;
* to assign field from config if it exists, otherwise leave the default field
  value: `field = json.As<T>(field)`.

To enable support for `std::optional`:
```cpp
#include <userver/formats/parse/common_containers.hpp>
```

#### Containers

To represent JSON arrays, use containers, typically:

* `std::vector<T>`
* `std::unordered_set<T>`
* `std::set<T>`

To represent JSON objects with unknown keys (OpenAPI's `additionalProperties`),
use map containers, typically:

* `std::unordered_map<std::string, T>`
* `std::map<std::string, T>`

To enable support for such containers, use the following:

```cpp
#include <userver/formats/parse/common_containers.hpp>
```

If the nested type is your custom type, make sure to define `Parse` for it:

```cpp
// To parse this type
std::unordered_map<std::string, std::optional<MyStruct>>

// You need to define this parser
MyStruct Parse(const formats::json::Value& value,
               formats::parse::To<MyStruct>);
```


#### Recommendations on parsing dynamic configs

1. Do not repeat the work of defining parsers for containers themselves.
   Define the parser for the inner type instead.

2. Do not hesitate to throw an exception if the provided config is invalid.
   There are
   @ref dynamic_config_fallback "multiple tiers of fallback mechanisms"
   in place to make sure it
   will not break the whole service. Exceptions are the preferred
   mechanism to signal this failure to the dynamic config system.

3. Make sure to check feasible invariants while parsing, e.g. minimum and
   maximum values of numbers.
   For that you occasionally may need to use
   @ref dynamic_config::Key::Key(std::string_view name, JsonParser parser, DefaultAsJsonString default_json)
   Don't leave strings that are semantically enums as strings, parse them
   to C++ `enum class`.

4. There is typically no need to be cautious about malicious input coming
   from the configs service. Still, in case of a human error, it's far better
   to fail a config update than to crash the service or to start silently
   ignoring the config.


@anchor dynamic_config_defaults
### Dynamic config defaults

Config defaults are specified in their
@ref dynamic_config_key "C++ definition".
They may be overridden in the static config of `dynamic-config` component:

@see @ref dynamic_config_defaults_override

If utils::DaemonMain is used, then the default dynamic configs can
be printed by passing `--print-dynamic-config-defaults` command line option.
It does not account for overrides in `dynamic-config` (components::DynamicConfig).

Dynamic config defaults are used in the following places:

1. In @ref dynamic_config_unit_tests "unit tests and benchmarks"

2. In @ref dynamic_config_testsuite "testsuite tests", defaults are the main
   source of configs (they can be overridden specifically for testsuite).

3. Unconditionally if config updates are not set up
   (`dynamic-config.updates-enabled: false`)

4. If `dynamic-config-client-updater` (components::DynamicConfigClientUpdater)
   (or a custom updater component) receives response from the configs service
   with some configs missing, then `dynamic-config` (components::DynamicConfig)
   fills in those configs from defaults.

5. If `dynamic-config` component loads the config cache file, and some configs
   are missing, then those are filled in from defaults.


@anchor dynamic_config_setup
### Setting up dynamic config

`dynamic-config` (components::DynamicConfig) itself is required
for the functioning of various core userver components, so it is included
in both components::MinimalComponentList and components::CommonComponentList.

By default, it is configured to function in the "static dynamic config" mode
and may be omitted from the static config entirely.


@anchor dynamic_config_defaults_override
#### Overriding dynamic configs defaults

If you rely on obtaining ground truth configs from a configs service
(as it is advised for best experience; described below),
then you may skip this section entirely and use defaults only for testing.

On the other hand, if you prefer to run without a configs service, then you
may want to override some of the
@ref scripts/docs/en/schemas/dynamic_configs.md "built-in userver configs".

Defaults can be overridden in the config of the `dynamic-config` component:

@snippet postgresql/functional_tests/integration_tests/static_config.yaml  dynamic config

In this case YAML is automatically converted to JSON, then parsed as usual.

Alternatively, you can pass the path to a JSON file with defaults:

```
# yaml
    dynamic-config:
        defaults-path: $dynamic-config-defaults
```

It somewhat complicates the deployment process, though.


@anchor dynamic_config_updates
#### Setting up dynamic config updates

Suppose that we've managed to persuade you to wind up a configs service and
start actively using dynamic configs.

You will need the following components to download configs:

* components::DynamicConfigClientUpdater
* components::DynamicConfigClient (used by the previous one)

Both are included in components::CommonComponentList.

Here is a reasonable static config for those:

@snippet samples/production_service/static_config.yaml Production service sample - static config dynamic configs


@anchor dynamic_config_fallback
### Fallback mechanisms for dynamic configs updates

Suppose that the new revision of the current service released before the newly
added config was accounted by the config service. In this case it should just
return the configs it knows about. The current service will fill in the blanks
from the defaults.

If the config service gives out invalid configs (which fail to be parsed),
then the periodic config update will fail, and alerts will fire:

1. `dynamic-config.parse-errors` metric (1) will be incremented, and
   `dynamic-config.was-last-parse-successful` metric (2) will be set to 1.
   You can combine those to safely detect parsing errors in the metrics
   backend:
   ```
   if ((1) != previous (1) || (2) == 0) show alert
   ```

2. An alert will be registered in alerts::StorageComponent, which can be
   accessed from outside using the optional alerts::Handler component

If the config service is not accessible at this point (down or overloaded),
then the periodic config update will also obviously fail.

After a config update failure, if the service has already started, then it will
keep using the previous successfully fetched configs. Also, you can monitor config update health using

```
cache.any.time.time-from-last-successful-start-ms: cache_name=dynamic-config-client-updater
```

and related metrics.

@see @ref scripts/docs/en/userver/service_monitor.md

If the first config update fails, then the service will read the config cache
file specified in `dynamic-config.fs-cache-path`, which is hopefully left since
the previous service start.

If the first config update fails, and the config cache file is missing, then
the service will fail to start, showing a helpful log message. Defaults are not
used in this case, because they may be significantly outdated, and to avoid
requiring the developer to always keep defaults up-to-date with production
requirements. Another reason for such behavior is that the dynamic configs are
used to fix up incidents, so such check of the dynamic configs service
at first deployment prevents incident escalation due to unnoticed
misconfiguration (missing routes to dynamic config service, broken
authorization, ...).

If you still wish to boot the service using just dynamic config defaults, you
can create a config cache file with contents `{}`, or bake a config cache file
into the service's Docker image.


@anchor dynamic_config_unit_tests
### Using dynamic config in unit tests and benchmarks

Main testing page:

@see @ref scripts/docs/en/userver/testing.md

dynamic_config::StorageMock stores configs for unit tests and benchmarks.
It must be kept alive while any dynamic_config::Source or
dynamic_config::Snapshot is accessing it.

Test code can obtain the @ref dynamic_config_defaults "default configs" using
dynamic_config::GetDefaultSnapshot or dynamic_config::GetDefaultSource, or
override some configs using dynamic_config::MakeDefaultStorage.


@anchor dynamic_config_testsuite
### Using dynamic config in testsuite

Main testsuite page:

@see @ref scripts/docs/en/userver/functional_testing.md

#### Overriding dynamic config for testsuite globally

Dynamic config can be overridden specifically for testsuite. It can be done
globally in the following ways:

1. Providing a JSON file with `--config-fallback` option to pytest. When using
   `userver_testsuite_add_simple` to setup tests in CMake, it is enough
   to place the `dynamic_config_fallback.json` file next to the static config.
2. Providing a patch directly in "Python JSON" format by overriding
   @ref dynamic_config_fallback_patch fixture

The various config patches are applied in the following order, going
from the lowest to the highest priority:

1. In-code defaults from @ref dynamic_config_key "dynamic_config::Key"
2. `dynamic-config.defaults` option
   from @ref dynamic_config_defaults_override "static config" , if any
3. JSON file passed to `--config-fallback`, if any
4. `dynamic_config_fallback_patch`, if any

#### Changing dynamic configs in testsuite per test

If the service has @ref dynamic_config_updates "config updates" disabled,
then there is no way to change configs per-test. You can resort to creating
multiple separate directories for testsuite tests and overriding the initial
dynamic config (as shown above) in each of those directories in different ways.

If the service has config updates enabled, then you can change dynamic config
per-test as follows:

```python
@pytest.mark.config(MY_CONFIG_NAME=42, MY_OTHER_CONFIG_NAME=True)
async def test_whatever(service_client, ...):
```

Dynamic config can also be modified mid-test using @ref dynamic_config fixture.

Such dynamic config changes are applied (sent to the service) at the first
`service_client` request in the test, or manually:

```python
await service_client.update_server_state()
```


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref rabbitmq_driver | @ref scripts/docs/en/schemas/dynamic_configs.md ⇨
@htmlonly </div> @endhtmlonly
