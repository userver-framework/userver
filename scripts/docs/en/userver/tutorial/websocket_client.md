# WebSocket Client

## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref scripts/docs/en/userver/tutorial/hello_service.md.

## Step by step guide

Let's write a simple HTTP handler that:
* receives `url` and `message` arguments
* connects to a WebSocket server by the `url` as a WebSocket client
* sends the `message`
* receives the response and closes the WebSocket connection
* returns the WebSocket response

### WebSocket handler

@snippet samples/websocket_client/main.cpp WebSocket client sample - handler

To create a WebSocket connection:
1. Use @ref clients::http::Request::PerformWebSocketHandshake to perform the handshake
2. Call @ref clients::http::WebSocketResponse::MakeWebSocketConnection on the response to get a @ref websocket::WebSocketConnection
3. Use @ref websocket::WebSocketConnection::SendText to send text messages, @ref websocket::WebSocketConnection::SendBinary to send binary messages
4. Use @ref websocket::WebSocketConnection::Recv to receive messages
5. Call @ref websocket::WebSocketConnection::Close when done

### Static config

Now we have to configure the service:

@include samples/websocket_client/static_config.yaml

### int main()

Finally, we add our component to the @ref components::MinimalServerComponentList.
Note that we use @ref clients::http::ComponentList which registers both `http-client` and `http-client-core` components:

@snippet samples/websocket_client/main.cpp WebSocket client sample - main

### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-websocket_client
```

The sample could be started by running
`make start-userver-samples-websocket_client`.

To start the service manually run
`./samples/websocket_client/userver-samples-websocket_client -c </path/to/static_config.yaml>`.

Now you can test the client by connecting it to a WebSocket echo server.
First, start the WebSocket server sample:
```
make start-userver-samples-websocket_service
```

Then, in another terminal, send a request to the client:
```
bash
$ curl 'http://localhost:8080/ws-client?url=ws://localhost:8080/chat&message=Hello'
Hello
```

### Functional testing

@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service
can be implemented using testsuite. The mockserver can emulate a WebSocket server
by returning `aiohttp.web.WebSocketResponse`:

@include samples/websocket_client/tests/test_websocket_client.py

Do not forget to add the plugin in conftest.py:

@include samples/websocket_client/tests/conftest.py

## Full sources

See the full example:
* @ref samples/websocket_client/main.cpp
* @ref samples/websocket_client/static_config.yaml
* @ref samples/websocket_client/CMakeLists.txt
* @ref samples/websocket_client/tests/conftest.py
* @ref samples/websocket_client/tests/test_websocket_client.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/websocket_service.md | @ref scripts/docs/en/userver/tutorial/static_content.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/websocket_client/main.cpp
@example samples/websocket_client/static_config.yaml
@example samples/websocket_client/CMakeLists.txt
@example samples/websocket_client/tests/conftest.py
@example samples/websocket_client/tests/test_websocket_client.py
