## Component system
 
A userver-based service usually consists of components. A component is a basic
building block that encapsulates dependencies logic with configuration and
is able to interact with other components.

\b Example:

* There is a `ClientB`, that requires initialization with some `SettingsB`
* There is a `ClientA`, that requires initialization with some `SettingsA` and
  a reference to `ClientB`.

```cpp
struct SettingsB{ /*...*/ };

class ClientB {
 public:
  ClientB(SettingsB settings);
};


struct SettingsA{ /*...*/ };

class ClientA {
 public:
  ClientA(ClientB& b, SettingsA settings);
};
```

In unit tests you could create the @ref userver_clients "clients" and fill the
settings manually in code:

```cpp
  ClientB b({.path="/opt/", .timeout=15s});
  ClientA a(b, {.ttl=3, .skip={"some"}});

  a.DoSomething();
```

When it comes to production services with tens of components and hundreds of
settings then the configuration from code becomes cumbersome and not flexible
enough. Knowledge about all the dependencies of clients is required; many
clients may depend on multiple other clients; some clients may be slow to start
and those should be initialized concurrently...

Components to the rescue! A component takes care of constructing and
configuring its own client. With components the configuration and dependency
management problem is decomposed into small and manageable pieces:

```cpp
class ComponentA: components::LoggableComponentBase {
 public:
  ComponentA(const components::ComponentConfig& config,
             const components::ComponentContext& context)
  : client_a_(
      context.FindComponent<ComponentB>().GetClientB(),
      {.ttl=config["ttl"].As<int>(), .skip=config["skip"].As<std::vector<std::string>>()}
    )
  {}

  ClientA& GetClientA() const { return client_a_; }

 private:
  ClientA client_a_;
};
```

Only components should know about components. Clients and other types
constructed by components should not use components::ComponentConfig,
components::ComponentContext, or components directly. All the components
should inherit from components::LoggableComponentBase base class and may
override its methods.

All the components are listed at the @ref userver_components API Group.

## Startup context
On component construction a components::ComponentContext is passed as a
second parameter to the constructor of the component. That context could
be used to get references to other components. That reference to the
component is guaranteed to outlive the component that is being constructed.

## Components construction and destruction order
utils::DaemonMain, components::Run or components::RunOnce
start all the components from the passed components::ComponentList.
Each component is constructed in a separate engine::Task on the default
task processor and is initialized concurrently with other components.

This is a useful feature, for example in cases
with multiple caches that slowly read from different databases.

To make component *A* depend on component *B* just call
components::ComponentContext::FindComponent<B>() in the constructor of A.
FindComponent() suspends the current task and continues only after the
construction of component B is finished. Components are destroyed
in reverse order of construction, so the component A is destroyed before
the component B. In other words - references from FindComponent() outlive
the component that called the FindComponent() function. If any component
loading fails, FindComponent() wakes up and throws an
components::ComponentsLoadCancelledException.

@anchor clients_from_components_lifetime
## References from components and lifetime of clients
It is a common practice to have a component that returns a reference *R* from
some function *F*. In such cases:
* a reference *R* lives as long as the component is alive
* a reference *R* is usually a client 
* and it is safe to invoke member functions of reference *R* concurrently
  unless otherwise specified.

Examples:
* components::HttpClient::GetHttpClient()
* components::StatisticsStorage::GetStorage()




## Components static configuration
components::ManagerControllerComponent configures the engine internals from
information provided in its static config: preallocates coroutines, creates
the engine::TaskProcessor, creates threads for low-level event processing.
After that it starts all the components that
were added to the components::ComponentList. Each registered component
should have a section in service config (also known as static config).

The component configuration is passed as a first parameter of type
components::ComponentConfig to the constructor of the component. Note that
components::ComponentConfig extends the functionality of
yaml_config::YamlConfig with YamlConfig::Mode::kEnvAllowed mode
that is able to substitute variables with values, use environment variales and
fallbacks. See yaml_config::YamlConfig for more info and examples.

All the components have the following options:

| Name         | Description                                        | Default value |
|--------------|----------------------------------------------------|---------------|
| load-enabled | set to `false` to disable loading of the component | true          |

@anchor static-configs-validation
### Static configs validation

To validate static configs you only need to define member function of your component 
`GetStaticConfigSchema()`

@snippet components/component_sample_test.cpp  Sample user component schema

All schemas and sub-schemas must have `description` field and can have 
`defaultDescription` field if they have a default value.

Scope of static config validatoin can be specified by `validate_all_components` section of 
`components_manager` config. To disable it use:

```
components_manager:
    static_config_validation:
        validate_all_components: false
```

You also can force static config validation of your component by adding `components::kHasValidate`

@snippet components/component_sample_test.hpp  Sample kHasValidate specialization

@note There are plans to use it to generate documentation.

Supported types:
* Scalars: `boolean`, `string`, `integer`, `double`
* `object` must have options `additionalProperties` and `properties`
* `array` must have option `items`

@anchor select-config-file-mode
### Setup config file mode
You can configure the configuration mode of the component in the configuration file
by specializing the `components::kConfigFileMode` template variable in the file with component declaration
Supported mode:
* `kConfigFileMode::kRequired` - The component must be defined in configuration file
* `kConfigFileMode::kNotRequired` - The component may not be defined in the configuration file

@snippet components/component_sample_test.hpp  Sample kConfigFileMode specialization

## Writing your own components
Users of the framework may (and should) write their own components.

Components provide functionality to tie the main part of the program with
the configuration and other components. Component should be lightweight
and simple.

@note Rule of a thumb: if you wish to unit test some code that is located
in the component, then in 99% of cases that code should not be located in
the component.

### Should I write a new component or class would be enough?
You need a component if:
* you need a static config
* you need to work with other components
* you are writing clients (you need a component to be the factory for your
clients)
* you want to subscribe for configs or cache changes

### HowTo
Start writing your component from adding a header file with a class
inherited from components::LoggableComponentBase.
@snippet components/component_sample_test.hpp  Sample user component header

In source file write the implementation of the component:
@snippet components/component_sample_test.cpp  Sample user component source
Destructor of the component is invoked on service shutdown. Components are
destroyed in the reverse order of construction. In other words, references from
`context.FindComponent<components::DynamicConfig>()` outlive the component.

If you need dynamic configs, you can get them using this approach:
@snippet components/component_sample_test.cpp  Sample user component runtime config source

@note See @ref scripts/docs/en/userver/tutorial/config_service.md for info on how to
implement your own config server.

Do not forget to register your component in components::ComponentList
before invoking the utils::DaemonMain, components::Run or
components::RunOnce.

Done! You've implemented your first component. Full sources:
* @ref components/component_sample_test.hpp
* @ref components/component_sample_test.cpp

@note For info on writing HTTP handler components refer to
the @ref scripts/docs/en/userver/tutorial/hello_service.md.

### Testing
Starting up the components in unit tests is quite hard. Prefer moving out
all the functionality from the component or testing the component with the
help of @ref scripts/docs/en/userver/functional_testing.md "testsuite functional tests".

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/auth_postgres.md | @ref userver_clients ⇨
@htmlonly </div> @endhtmlonly

@example components/component_sample_test.hpp
@example components/component_sample_test.cpp
