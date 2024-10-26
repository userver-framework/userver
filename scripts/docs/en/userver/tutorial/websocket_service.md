# Writing your WebSocket service

## Before you start

Make sure that you can compile and run core tests as described at
@ref scripts/docs/en/userver/build/build.md.

## Step by step guide

Typical WebSocket server application in userver consists of the following parts:
* WebSocket handler component - main logic of your application
* Static config - startup config that does not change for the whole lifetime of an application
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


### int main()

Finally, we
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
`./samples/websocket_service/userver-samples-websocket_service -c </path/to/static_config.yaml>`.

@note Without file path to `static_config.yaml` `userver-samples-websocket_service` will look for a file with name `config_dev.yaml`
@note CMake doesn't copy `static_config.yaml` file from `samples` directory into build directory.

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
* @ref samples/websocket_service/CMakeLists.txt
* @ref samples/websocket_service/tests/conftest.py
* @ref samples/websocket_service/tests/test_websocket.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/digest_auth_postgres.md | @ref scripts/docs/en/userver/tutorial/multipart_service.md ⇨
@htmlonly </div> @endhtmlonly


@example samples/websocket_service/websocket_service.cpp
@example samples/websocket_service/static_config.yaml
@example samples/websocket_service/CMakeLists.txt
@example samples/websocket_service/tests/conftest.py
@example samples/websocket_service/tests/test_websocket.py

