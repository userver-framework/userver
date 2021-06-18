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
* components::TaxiConfigClientUpdater and components::TaxiConfigClient components should be added to the service component list
* the above components should be configured to retrieve dynamic configs from the configuration service:

@snippet samples/config_service.cpp Config service sample - config updater static config

Now let's create a configuration service.


### HTTP handler component

Dynamic configs are requested via JSON requests, so we need to create a simple JSON handler.

There are two ways to write a JSON handler:
* We could do that by creating a component derived from server::handlers::HttpHandlerBase as in the @ref md_en_userver_tutorial_hello_service example. In that case we would need
    * to parse the input via formats::json::FromString
    * and to send the result by serializing a JSON to std::string via formats::json::ToString.
* Or we could just take a server::handlers::HttpHandlerJsonBase that does all the above steps for us.

We are going to take the second approach:

@snippet samples/config_service.cpp Config service sample - component

@warning `Handle*` functions are invoked concurrently on the same instance of the handler class. Use @ref md_en_userver_synchronization "synchronization primitives" or do not modify shared data in `Handle*`.

Note the rcu::Variable. There may be (and will be!) multiple concurrent requests and the `HandleRequestJsonThrow` would be invoked concurrently on the same instance of `ConfigDistributor`. The rcu::Variable allows us to atomically update the config value, concurrently with the `HandleRequestJsonThrow` invocations.


### HandleRequestJsonThrow

All the interesting things happen in the `HandleRequestJsonThrow` function, where we grab a rcu::Variable snapshot, fill the update time and the configuration from it:

@snippet samples/config_service.cpp Config service sample - HandleRequestJsonThrow

The "configs" field is formed in the `MakeConfigs` function depending on the request parameters:

@snippet samples/config_service.cpp Config service sample - MakeConfigs

Note that the service name is sent in the "service" field of the JSON request body. Using it you can make **service specific dynamic configs**.

### Static config

Now we have to configure our new HTTP handle. The configuration is quite straightforward:

@snippet samples/config_service.cpp Config service sample - config updater static config


### int main()

Finally, let's write down the runtime config `kRuntimeConfig` as a fallback config,
adding required components to the `components::MinimalServerComponentList()`,
and starting the server with static config `kStaticConfig`.

@snippet samples/config_service.cpp  Config service sample - main

### Build
To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-config_service
```

Start the server by running `./samples/userver-samples-config_service`.
Now you can send a request to your server from another terminal:
```
$ curl -X POST -d '{}' 127.0.0.1:8083/configs/values | jq
{
  "configs": {
    "HTTP_CLIENT_CONNECT_THROTTLE": {
      "max-size": 100,
      "token-update-interval-ms": 0
    },
    "HTTP_CLIENT_CONNECTION_POOL_SIZE": 1000,
    "HTTP_CLIENT_ENFORCE_TASK_DEADLINE": {
      "cancel-request": false,
      "update-timeout": false
    },
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
    "USERVER_HTTP_PROXY": ""
  },
  "updated_at": "2021-06-16T09:25:36.10599711+0000"
}

$ curl -X POST -d '{"ids":["HTTP_CLIENT_CONNECT_THROTTLE"]}' 127.0.0.1:8083/configs/values | jq
{
  "configs": {
    "HTTP_CLIENT_CONNECT_THROTTLE": {
      "max-size": 100,
      "token-update-interval-ms": 0
    }
  },
  "updated_at": "2021-06-16T09:25:36.10599711+0000"
}
```

## Full sources

See the full example at @ref samples/config_service.cpp
@example samples/config_service.cpp
