# Writing your WebSocket service

## Before you start

Make sure that you can compile and run core tests as described at
@ref scripts/docs/en/userver/tutorial/build.md.

## Step by step guide

Typical WebSocket server application in userver consists of the following parts:
* WebSocket handler component - main logic of your application
* Static config - startup config that does not change for the whole lifetime of an application
* Dynamic config - config that could be changed at runtime
* int main() - startup code

Let's write a simple chat server that echoes incoming messages as outgoing messages for any websocket connected to `/chat` URL.

### WebSocket handler component

WebSocket handlers must derive from `server::websocket::WebsocketHandlerBase` and have a name, that
is obtainable at compile time via `kName` variable and is obtainable at runtime via `HandlerName()`.

@snippet samples/websocket_service/websocket_service.cpp  Websocket service sample - component

### Static config

Now we have to configure the service by providing `task_processors` and
`default_task_processor` options for the components::ManagerControllerComponent
and configuring each component in `components` section:

@include samples/websocket_service/static_config.yaml

Note that all the @ref userver_components "components" and
@ref userver_http_handlers "handlers" have their static options additionally
described in docs.

### Dynamic config

We are not planning to get new dynamic config values in this sample. Because of
that we just write the defaults to the fallback file of
the `components::DynamicConfigFallbacks` component.

All the values are described in a separate section @ref scripts/docs/en/schemas/dynamic_configs.md .

@include samples/websocket_service/dynamic_config_fallback.json

A production ready service would dynamically retrieve the above options at runtime from a configuration service. See
@ref scripts/docs/en/userver/tutorial/config_service.md for insights on how to change the
above options on the fly, without restarting the service.


### int main()

Finally, after writing down the dynamic config values into file at `dynamic-config-fallbacks.fallback-path`, we
add our component to the `components::MinimalServerComponentList()`,
and start the server with static configuration file passed from command line.

@snippet samples/websocket_service/websocket_service.cpp  Websocket service sample - main

### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-websocket_service
```

The sample could be started by running
`make start-userver-samples-websocket_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/websocket_service/userver-samples-websocket_service -c </path/to/static_config.yaml>`
(do not forget to prepare the configuration files!).

@note Without file path to `static_config.yaml` `userver-samples-websocket_service` will look for a file with name `config_dev.yaml`
@note CMake doesn't copy `static_config.yaml` and `dynamic_config_fallback.json` files from `samples` directory into build directory.

Now you can send messages to your server from another terminal:
```
bash
$ wscat --connect ws://localhost:8080/chat
Connected (press CTRL+C to quit)
> hello
< hello
```

### Functional testing

@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the @ref websocket_client "websocket_client" fixture from
pytest_userver.plugins.core in the
following way:

@snippet samples/websocket_service/tests/test_websocket.py  Functional test

Do not forget to add the plugin in conftest.py:

@snippet samples/websocket_service/tests/conftest.py  registration

## Full sources

See the full example at:
* @ref samples/websocket_service/websocket_service.cpp
* @ref samples/websocket_service/static_config.yaml
* @ref samples/websocket_service/dynamic_config_fallback.json
* @ref samples/websocket_service/CMakeLists.txt
* @ref samples/websocket_service/tests/conftest.py
* @ref samples/websocket_service/tests/test_websocket.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/digest_auth_postgres.md | @ref scripts/docs/en/userver/tutorial/multipart_service.md ⇨
@htmlonly </div> @endhtmlonly


@example samples/websocket_service/websocket_service.cpp
@example samples/websocket_service/static_config.yaml
@example samples/websocket_service/dynamic_config_fallback.json
@example samples/websocket_service/CMakeLists.txt
@example samples/websocket_service/tests/conftest.py
@example samples/websocket_service/tests/test_websocket.py

