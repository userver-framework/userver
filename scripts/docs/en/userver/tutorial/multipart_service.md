## File uploads and multipart/form-data testing

## Before you start

Make sure that you can compile and run core tests as described at
@ref scripts/docs/en/userver/build/build.md.

Take a look at the @ref scripts/docs/en/userver/tutorial/hello_service.md
and make sure that you do realize the basic concepts.


## Step by step guide

It is a common task to upload an image or even work with multiple HTML form
parameters at once.

An [OpenAPI scheme](https://swagger.io/docs/specification/describing-request-body/multipart-requests/)
for such form could look like:

```
# yaml
requestBody:
  content: 
    multipart/form-data: # Media type
      schema:            # Request payload
        type: object
        properties:      # Request parts
          address:       # Part1 (object)
            type: object
            properties:
              street:
                type: string
              city:
                type: string
          profileImage:  # Part 2 (an image)
            type: string
            format: binary
```

Here we have `multipart/form-data` request with a JSON object and a file.

Let's implement a `/v1/multipart` handler for that scheme and do something with
each of the request parts!


### HTTP handler component

Just like in the @ref scripts/docs/en/userver/tutorial/hello_service.md
the handler itself is a component inherited from
server::handlers::HttpHandlerBase:

@snippet samples/multipart_service/service.cpp  Multipart service sample - component

The primary functionality of the handler should be located in
`HandleRequestThrow` function. To work with the `multipart/form-data`
parameters use the appropriate
server::http::HttpRequest functions:

* server::http::HttpRequest::FormDataArgCount()
* server::http::HttpRequest::FormDataArgNames()
* server::http::HttpRequest::GetFormDataArg()
* server::http::HttpRequest::GetFormDataArgVector()
* server::http::HttpRequest::HasFormDataArg()

@snippet samples/multipart_service/service.cpp  Multipart service sample - HandleRequestThrow

Note the work with the `image` in the above snippet. The image has a
binary representation that require no additional decoding. The bytes of a
received image match the image bytes on hard-drive.

JSON data is received as a string. FromString function converts it to DOM
representation.

@warning `Handle*` functions are invoked concurrently on the same instance of
  the handler class. Use @ref scripts/docs/en/userver/synchronization.md "synchronization primitives"
  or do not modify shared data in `Handle*`.


### Static config

Now we have to configure the service by providing `task_processors` and
`default_task_processor` options for the components::ManagerControllerComponent and
configuring each component in `components` section:

@include samples/multipart_service/static_config.yaml

Note that all the @ref userver_components "components" and
@ref userver_http_handlers "handlers" have their static options additionally
described in docs.


### int main()

Finally, we
add our component to the `components::MinimalServerComponentList()`,
and start the server with static configuration file passed from command line.

@snippet samples/multipart_service/service.cpp  Multipart service sample - main


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-multipart_service
```

The sample could be started by running
`make start-userver-samples-hello_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/multipart_service/userver-samples-multipart_service -c </path/to/static_config.yaml>`.

@note Without file path to `static_config.yaml` `userver-samples-multipart_service` will look for a file with name `config_dev.yaml`
@note CMake doesn't copy `static_config.yaml` files from `samples` directory into build directory.

Now you can send a request to your server from another terminal:
```
bash
$ curl -v -F address='{"street": "3, Garden St", "city": "Hillsbery, UT"}' \
          -F "profileImage=@../scripts/docs/logo_in_circle.png" \
          http://localhost:8080/v1/multipart
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> POST /v1/multipart HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.81.0
> Accept: */*
> Content-Length: 10651
> Content-Type: multipart/form-data; boundary=------------------------048363632fdb9acc
> 
* We are completely uploaded and fine
* Mark bundle as not supporting multiuse
< HTTP/1.1 200 OK
< Date: Tue, 24 Oct 2023 09:31:42 UTC
< Content-Type: application/octet-stream
< Server: userver/2.0 (20231024091132; rv:unknown)
< X-YaRequestId: f7cb383a987248179e5683713b141cea
< X-YaTraceId: 7976203e08074091b20b08738cb7fadc
< X-YaSpanId: 29233f54bc7e5009
< Accept-Encoding: gzip, identity
< Connection: keep-alive
< Content-Length: 76
< 
* Connection #0 to host localhost left intact
city=Hillsbery, UT image_size=10173
```

### Functional testing

@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the @ref service_client "service_client" fixture from
pytest_userver.plugins.core in the
following way:

@snippet samples/multipart_service/tests/test_multipart.py  Functional test

Do not forget to add the plugin in conftest.py:

@snippet samples/multipart_service/tests/conftest.py  registration

## Full sources

See the full example at:
* @ref samples/multipart_service/service.cpp
* @ref samples/multipart_service/static_config.yaml
* @ref samples/multipart_service/CMakeLists.txt
* @ref samples/multipart_service/tests/conftest.py
* @ref samples/multipart_service/tests/test_multipart.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/websocket_service.md | @ref scripts/docs/en/userver/tutorial/json_to_yaml.md ⇨
@htmlonly </div> @endhtmlonly


@example samples/multipart_service/service.cpp
@example samples/multipart_service/static_config.yaml
@example samples/multipart_service/CMakeLists.txt
@example samples/multipart_service/tests/conftest.py
@example samples/multipart_service/tests/test_multipart.py

