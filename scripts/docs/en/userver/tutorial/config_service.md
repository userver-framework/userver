## Writing your own configs server

## Before you start

Make sure that you can compile and run core tests and read a basic example @ref md_en_userver_tutorial_hello_service.

## Step by step guide

An ability to change service behavior at runtime without restarting the service is priceless! We have that ability, it is called dynamic configs and it allows you:

* to control logging (@ref USERVER_NO_LOG_SPANS, @ref USERVER_LOG_REQUEST, @ref USERVER_LOG_REQUEST_HEADERS)
* to control RPS and deal with high loads (@ref HTTP_CLIENT_CONNECT_THROTTLE, @ref USERVER_RPS_CCONTROL, @ref USERVER_TASK_PROCESSOR_QOS)
* to dynamically switch from one HTTP proxy to another or turn off proxying (@ref USERVER_HTTP_PROXY)
* to write your own runtime dynamic configs:
  * to create experiments that could be adjusted or turned on/off without service restarts
  * to do whatever you like

In previous example we made a simple HTTP server with some dynamic configs set in stone. To make the dynamic configs dynamic for real the following steps should be done:
* components::DynamicConfigClientUpdater and components::DynamicConfigClient
  components should be added to the service component list
* the above components should be configured to retrieve dynamic configs from the
  configuration service:

```
    # yaml
    dynamic-config-client:               # A client that knows how to request configs via HTTP
        config-url: http://localhost:8083/  # URL of dynamic config service
        http-retries: 5
        http-timeout: 20s
        service-name: configs-service
        fallback-to-no-proxy: false    # On error do not attempt to retrieve configs 
                                       # by bypassing proxy from USERVER_HTTP_PROXY dynamic config

    dynamic-config-client-updater:        # A component that periodically uses `dynamic-config-client` to retrieve new values
        fallback-path: /var/cache/service-name/dynamic_cfg.json  # Fallback to the values from this file on error
        load-only-my-values: true      # Do not request all the configs, only the ask for the ones we are using right 
        store-enabled: true            # Store the retrieved values into the components::DynamicConfig
        update-interval: 5s            # Request for new configs every 5 seconds
        full-update-interval: 1m
        config-settings: false
```

Now let's create a configuration service.


### HTTP handler component

Dynamic configs are requested via JSON requests, so we need to create a simple JSON handler that is responding with config values.

There are two ways to write a JSON handler:
* We could do that by creating a component derived from server::handlers::HttpHandlerBase as in the @ref md_en_userver_tutorial_hello_service example. In that case we would need
    * to parse the input via formats::json::FromString
    * and to send the result by serializing a JSON to std::string via formats::json::ToString.
* Or we could just take a server::handlers::HttpHandlerJsonBase that does all the above steps for us.

We are going to take the second approach:

@snippet samples/config_service/config_service.cpp Config service sample - component

@warning `Handle*` functions are invoked concurrently on the same instance of the handler class. Use @ref md_en_userver_synchronization "synchronization primitives" or do not modify shared data in `Handle*`.

Note the rcu::Variable. There may be (and will be!) multiple concurrent requests and the `HandleRequestJsonThrow` would be invoked concurrently on the same instance of `ConfigDistributor`. The rcu::Variable allows us to atomically update the config value, concurrently with the `HandleRequestJsonThrow` invocations.

Function `ConfigDistributor::SetNewValues` is meant for setting config values to send. For example, you can write another component that accepts JSON requests with new config values and puts those values into `ConfigDistributor` via `SetNewValues(new_values)`.

### HandleRequestJsonThrow

All the interesting things happen in the `HandleRequestJsonThrow` function, where we grab a rcu::Variable snapshot, fill the update time and the configuration from it:

@snippet samples/config_service/config_service.cpp Config service sample - HandleRequestJsonThrow

The "configs" field is formed in the `MakeConfigs` function depending on the request parameters:

@snippet samples/config_service/config_service.cpp Config service sample - MakeConfigs

Note that the service name is sent in the "service" field of the JSON request body. Using it you can make **service specific dynamic configs**.

### Static config

Now we have to configure our new HTTP handle. The configuration is quite straightforward:

@snippet samples/config_service/static_config.yaml Config service sample - handler static config


### int main()

Finally, 
we add required components to the `components::MinimalServerComponentList()`,
and start the server with static config `kStaticConfig`.

@snippet samples/config_service/config_service.cpp  Config service sample - main


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
bash
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-config_service
```

The sample could be started by running
`make start-userver-samples-config_service`. The command would invoke
@ref md_en_userver_functional_testing "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/config_service/userver-samples-config_service -c </path/to/static_config.yaml>`
(do not forget to prepare the configuration files!).

Now you can send a request to your server from another terminal:
```
bash
$ curl -X POST -d '{}' 127.0.0.1:8083/configs/values | jq
{
  "configs": {
    "USERVER_DUMPS": {},
    "USERVER_LRU_CACHES": {},
    "USERVER_CACHES": {},
    "USERVER_LOG_REQUEST": true,
    "USERVER_TASK_PROCESSOR_QOS": {
      "default-service": {
        "default-task-processor": {
          "wait_queue_overload": {
            "action": "ignore",
            "length_limit": 5000,
            "time_limit_us": 3000
          }
        }
      }
    },
    "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
    "USERVER_LOG_REQUEST_HEADERS": true,
    "USERVER_CHECK_AUTH_IN_HANDLERS": false,
    "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE": false,
    "USERVER_HTTP_PROXY": ""
  },
  "updated_at": "2021-06-29T14:15:31.173239295+0000"
}


$ curl -X POST -d '{"ids":["USERVER_TASK_PROCESSOR_QOS"]}' 127.0.0.1:8083/configs/values | jq
{
  "configs": {
    "USERVER_TASK_PROCESSOR_QOS": {
      "default-service": {
        "default-task-processor": {
          "wait_queue_overload": {
            "action": "ignore",
            "length_limit": 5000,
            "time_limit_us": 3000
          }
        }
      }
    }
  },
  "updated_at": "2021-06-29T14:15:31.173239295+0000"
}
```

### Functional testing
@ref md_en_userver_functional_testing "Functional tests" for the service
could be implemented using the @ref service_client "service_client fixture"
in the following way:

@snippet samples/config_service/tests/test_config.py  Functional test

Do not forget to add the plugin in conftest.py:

@snippet samples/config_service/tests/conftest.py  registration


## Ready to use uservice-dynconf

Note that there is a ready to use opensource
[uservice-dynconf](https://github.com/userver-framework/uservice-dynconf)
dynamic configs service. Use it for your projects or just disable
dynamic config updates and keep developing without a supplementary service.

## Full sources

See the full example:
* @ref samples/config_service/config_service.cpp
* @ref samples/config_service/static_config.yaml
* @ref samples/config_service/dynamic_config_fallback.json
* @ref samples/config_service/CMakeLists.txt
* @ref samples/config_service/tests/conftest.py
* @ref samples/config_service/tests/test_config.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_tutorial_hello_service | @ref md_en_userver_tutorial_production_service ⇨
@htmlonly </div> @endhtmlonly



@example samples/config_service/config_service.cpp
@example samples/config_service/static_config.yaml
@example samples/config_service/dynamic_config_fallback.json
@example samples/config_service/CMakeLists.txt
@example samples/config_service/tests/conftest.py
@example samples/config_service/tests/test_config.py
