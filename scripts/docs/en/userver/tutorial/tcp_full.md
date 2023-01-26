## TCP full-duplex server with metrics and Spans


## Before you start

Make sure that you can compile and run core tests as described at
@ref md_en_userver_tutorial_build and took a look at the
@ref md_en_userver_tutorial_tcp_service.


## Step by step guide

Let's write a TCP echo server. It should accept incoming connections, read the
data from socket and send the received data back concurrently with read. The
read/write operation continues as long as the socket is open.

We would also need production quality metrics and logs for the service. 


### TCP server

Derive from components::TcpAcceptorBase and override the `ProcessSocket`
function to get the new sockets:

@snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - component

@warning `ProcessSocket` functions are invoked concurrently on the same 
instance of the class. Use @ref md_en_userver_synchronization "synchronization primitives"
or do not modify shared data in `ProcessSocket`.

`struct Stats` holds the statistics for the component and is defined as:

@snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - Stats definition


### Statistics registration

To automatically deliver the metrics they should be registered via
utils::statistics::MetricTag and DumpMetric+ResetMetric functions should be
defined:

@snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - Stats tag

Now the tag could be used in component constructor to get a reference to the
`struct Stats`:

@snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - constructor


### Static config

Lets configure our component in the `components` section:

@snippet samples/tcp_full_duplex_service/static_config.yaml  TCP component

We also need to configure the HTTP server and the handle that responds with
statistics:

@snippet samples/tcp_full_duplex_service/static_config.yaml  HTTP and stats


### Dynamic config

We are not planning to get updates for dynamic config values in this sample. Because of
that we just write the defaults to the fallback file of
the `components::DynamicConfigFallbacks` component.

All the values are described in a separate section @ref md_en_schemas_dynamic_configs .

@include samples/tcp_full_duplex_service/dynamic_config_fallback.json

A production ready service would dynamically retrieve the above options at
runtime from a configuration service. See
@ref md_en_userver_tutorial_config_service for insights on how to change the
above options on the fly, without restarting the service.


### ProcessSocket

The full-duplex communication means that the same engine::io::Socket is
concurrently used for sending and receiving data. It is safe to concurrently
read and write into socket. We would need two functions:
* function that reads data from socket and puts it into a queue
* function that pops data from queue and sends it

Those two functions could be implemented in the following way:

@snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - SendRecv

Now it's time to handle new sockets. In the ProcessSocket function consists of
the following steps:
* increment the stats for opened sockets
* create a span with a "fd" tag to trace the sockets
* make a guard to increment the closed sockets statistics on scope exit
* create a queue for messages
* create a new task that sends the messages from the queue
* run the receiving function

@snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - ProcessSocket

The tracing::Span and utils::Async work together to produce nice logs that
allow you to trace particular file descriptor:
```
tskv	timestamp=2022-08-22T16:31:34.855853	text=Failed to read data	fd=108	link=5bc8829cc3dc425d8d5c5d560f815fa2	trace_id=63eb16f2165d45669c23df725530572c	span_id=17b35cd05db1c11e
``` 

On scope exit (for example because of the exception or return) the destructors
would work in the following order:
* destructor of the producer - it unblocks the consumer Pop operation
* destructor of `send_task` - it cancels the coroutine and waits for it finish
* destructor of consumer
* destructor of queue
* destructor of scope guard - it increments the closed sockets count
* destructor of span - it writes the execution time of the scope
* destructor of socket is called after leaving the ProcessSocket - it closes
  the OS socket.


### int main()

Finally, add the component to the `components::MinimalServerComponentList()`,
and start the server with static configuration file passed from command line.

@snippet samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp  TCP sample - main


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-tcp_full_duplex_service
```

The sample could be started by running
`make start-userver-samples-tcp_full_duplex_service`. The command would invoke
@ref md_en_userver_functional_testing "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/tcp_full_duplex_service/userver-samples-tcp_full_duplex_service -c </path/to/static_config.yaml>`
(do not forget to prepare the configuration files!).

Now you can send a request to your server from another terminal:
```
bash
$ nc localhost 8181
hello
hello
test test test
test test test
```

### Functional testing
@ref md_en_userver_functional_testing "Functional tests" for the service and
its metrics could be implemented using the testsuite in the following way:

@snippet samples/tcp_full_duplex_service/tests/test_echo.py  Functional test


Note that in this case testsuite requires some help to detect that the service
is ready to accept requests. To do that, override the
@ref pytest_userver.plugins.service.service_non_http_health_checks "service_non_http_health_checks":

@snippet samples/tcp_full_duplex_service/tests/conftest.py  service_non_http_health_checker


## Full sources

See the full example at:
* @ref samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp
* @ref samples/tcp_full_duplex_service/static_config.yaml
* @ref samples/tcp_full_duplex_service/dynamic_config_fallback.json
* @ref samples/tcp_full_duplex_service/CMakeLists.txt
* @ref samples/tcp_full_duplex_service/tests/conftest.py
* @ref samples/tcp_full_duplex_service/tests/test_echo.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_tutorial_tcp_service | @ref md_en_userver_tutorial_http_caching ⇨
@htmlonly </div> @endhtmlonly

@example samples/tcp_full_duplex_service/tcp_full_duplex_service.cpp
@example samples/tcp_full_duplex_service/static_config.yaml
@example samples/tcp_full_duplex_service/dynamic_config_fallback.json
@example samples/tcp_full_duplex_service/CMakeLists.txt
@example samples/tcp_full_duplex_service/tests/conftest.py
@example samples/tcp_full_duplex_service/tests/test_echo.py

