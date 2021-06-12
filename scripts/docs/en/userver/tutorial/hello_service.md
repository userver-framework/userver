## Writing your first HTTP server

## Before you start

Make sure that you can compile and run core tests.

## Step by step guide

Typical HTTP server application in userver consists of the following parts:
* HTTP handler component - main logic of your application
* Static config - startup config that does not change for the whole lifetime of an application
* Runtime config - config that could be changed at runtime
* int main() - startup code

Let's write a simple server that responds with "Hello world!\n" on every request to `/hello` URL.

### HTTP handler component

HTTP handlers must derive from `server::handlers::HttpHandlerBase` and have a name, that 
is obtainable at compile time via `kName` variable and is obtainable at runtime via `HandlerName()`.

The primary functionality of the handler should be located in `HandleRequestThrow` function.
Return value of this function is the HTTP response body. If an exception `exc` derived from
`server::handlers::CustomHandlerException` is thrown from the function then the
HTTP response code will be set to `exc.GetCode()` and `exc.GetExternalErrorBody()`
would be used for HTTP response body. Otherwise if the an exception `exc` derived from
`std::exception` is thrown from the function then the
HTTP response code will be set to `500`.

@snippet samples/hello_service.cpp  Hello service sample - component


### Static config

Now we have to configure the service and the handler:

@snippet samples/hello_service.cpp  Hello service sample - static config

Note that all the @ref userver_components "components" and @ref userver_http_handlers "handlers" have their static options additionally described in docs.

### Runtime config

We are not planning to get new runtime config values in this sample. Because of
that we just write the defaults to the bootstrap and fallback files of the
`components::TaxiConfig` component.

All the values are described in a separate section @ref md_en_schemas_dynamic_configs .

@snippet samples/hello_service.cpp  Hello service sample - runtime config


### int main()

Finally, writing down the runtime config `kRuntimeConfig` as a fallback config, 
adding our component to the `components::MinimalServerComponentList()`,
and starting the server with static config `kStaticConfig`. 

@snippet samples/hello_service.cpp  Hello service sample - main

### Build
To build the sample execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-hello_service
```

Start the server by running `./samples/userver-samples-hello_service`.
Now you can request your server from another terminal:
```
$ curl 127.0.0.1:8080/hello
Hello world!
```

## Full sources

See the full example at @ref samples/hello_service.cpp
@example samples/hello_service.cpp
