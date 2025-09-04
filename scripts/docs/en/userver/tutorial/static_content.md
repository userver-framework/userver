## Serving Static Content and Dynamic Web Pages

## Before you start

Make sure that you can compile and run core tests as described at
@ref scripts/docs/en/userver/build/build.md.

This sample demonstrates how to serve static files (HTML, CSS, JavaScript, images, etc.) using 🐙 userver and how
to create a client-side dynamic web page. This is a common requirement for web services that need to provide a web
interface, documentation, or other static assets alongside their dynamic API.


## Step by step guide

Let's create a single page web application that
* on POST request to `/v1/messages` remembers the JSON request body that consists of JSON object
  `{"name": "some-name", "message": "the message"}`;
* on GET request to `/v1/messages` returns all the messages as JSON array of
  `{"name": "some-name", "message": "the message"}` objects;
* serves static HTML, CSS and JavaScript files
* static HTML uses trivial JavaScript to create a client-side dynamic page using requests to the `/v1/messages`.

In this example we won't use database for simplicity of the sample.


### Application Logic

Our application logic is straightforward, we'll implement it in a single file. First we start with messages parsing
and serialization:

@snippet samples/static_service/main.cpp  Static service sample - Message

For more information on `Parse` and `Serialize` customization points see @ref scripts/docs/en/userver/formats.md. In
more mature services it is recommended to use @ref scripts/docs/en/userver/chaotic.md instead of
manual parsing.

Next step is to define the handler class:

@snippet samples/static_service/main.cpp  Static service sample - MessagesHandler

In the above code we have to use @ref concurrent::Variable, because the `HandleRequestJsonThrow` function is called
concurrently on the same instance of `MessagesHandler` if more than one requests is performed at the same time.


### Serving Static Content

The C++ code for serving static content is extremely simple because all the logic is provided by the ready-to-use
@ref userver::server::handlers::HttpHandlerStatic. Just add it and the components::FsCache into the component list:

@snippet samples/static_service/main.cpp  Static service sample - main


### Static config

Providing paths to files in file system to the `fs-cache-main` component, telling the `handler-static` to work with
`fs-cache-main` and configuring our `MessagesHandler` handler:

@snippet samples/static_service/static_config.yaml  static config

With the above config a request to
* `/index.html` will receive file `/var/www/index.html`;
* `/` will receive file `/var/www/index.html` due to the default value of `directory-file` static config option of
  @ref userver::server::handlers::HttpHandlerStatic;
* `/custom.js` will receive file `/var/www/custom.js`;
* `/custom.css` will receive file `/var/www/custom.css`;
* `/v1/messages` request will be processed by our `MessagesHandler` handler.


### Dynamic Web Page

In this example the logic related to `/v1/messages` requests is moved into a separate `custom.js` file:

@include samples/static_service/public/custom.js

The `index.html` page after DOM content is loaded fetches the existing messages. On button click new message is sent
to the server and the page is reloaded:

@include samples/static_service/public/index.html

More realistic applications usually use some JavaScript framework to manage data. For such example see
the [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf). Another example is the
[upastebin](https://github.com/userver-framework/upastebin) implementation.


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-static_service
```

The sample could be started by running `make start-userver-samples-static_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files and starts the service. After a prompt:
```
====================================================================================================
Started service at http://localhost:8080/
Monitor URL is http://localhost:-1/
====================================================================================================
```
open the address in web browser to play with the example:

![example](/sample_static_service.png)

To start the service manually run
`./samples/hello_service/userver-samples-hello_service -c </path/to/static_config.yaml>`.

@note CMake doesn't copy `static_config.yaml` and file from `samples` directory into build directory.
      As an example see [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf) for an insights
      of how to update `CMakeLists.txt` and how to test installation in CI.


### Functional testing

In `conftest.py` path to the directory with static content should be adjusted:

@include samples/static_service/testsuite/conftest.py

After that,  @ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the @ref service_client "service_client" fixture from pytest_userver.plugins.core in the
following way:

@snippet samples/static_service/testsuite/test_static.py  Functional test


## Full sources

See the full example at:
* @ref samples/static_service/static_config.yaml
* @ref samples/static_service/main.cpp
* @ref samples/static_service/CMakeLists.txt
* @ref samples/static_service/public/custom.css
* @ref samples/static_service/public/custom.js
* @ref samples/static_service/public/index.html
* @ref samples/static_service/testsuite/conftest.py
* @ref samples/static_service/testsuite/test_static.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/websocket_service.md | @ref scripts/docs/en/userver/tutorial/multipart_service.md ⇨
@htmlonly </div> @endhtmlonly


@example samples/static_service/static_config.yaml
@example samples/static_service/main.cpp
@example samples/static_service/CMakeLists.txt
@example samples/static_service/public/custom.css
@example samples/static_service/public/custom.js
@example samples/static_service/public/index.html
@example samples/static_service/testsuite/conftest.py
@example samples/static_service/testsuite/test_static.py

