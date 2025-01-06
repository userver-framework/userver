## TCP half-duplex server with static configs validation

## Before you start

Make sure that you can compile and run core tests as described at
@ref scripts/docs/en/userver/build/build.md.


## Step by step guide

Let's write a simple TCP server that accepts incoming connections, and as long
as the client sends "hi" responds with greeting from configuration file.


### TCP server

Derive from components::TcpAcceptorBase and override the `ProcessSocket`
function to get the new sockets:

@snippet samples/tcp_service/tcp_service.cpp  TCP sample - component

@warning `ProcessSocket` functions are invoked concurrently on the same 
instance of the class. Use @ref scripts/docs/en/userver/synchronization.md "synchronization primitives"
or do not modify shared data in `ProcessSocket`.


### Static config

Our new "tcp-hello" component should support the options of the components::TcpAcceptorBase
and the "greeting" option. To achieve that we would need the following
implementation of the `GetStaticConfigSchema` function:

@snippet samples/tcp_service/tcp_service.cpp  TCP sample - GetStaticConfigSchema

Now lets configure our component in the `components` section:

@snippet samples/tcp_service/static_config.yaml  TCP component


### ProcessSocket

It's time to deal with new sockets. The code is quite straightforward:

@snippet samples/tcp_service/tcp_service.cpp  TCP sample - ProcessSocket


### int main()

Finally, add the component to the `components::MinimalComponentList()`,
and start the server with static configuration file passed from command line.

@snippet samples/tcp_service/tcp_service.cpp  TCP sample - main


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-tcp_service
```

The sample could be started by running
`make start-userver-samples-tcp_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/tcp_service/userver-samples-tcp_service -c </path/to/static_config.yaml>`.

Now you can send a request to your server from another terminal:
```
bash
$ nc localhost 8180
hi
hello
```

### Functional testing
@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the testsuite in the following way:

@snippet samples/tcp_service/tests/test_tcp.py  Functional test


Note that in this case testsuite requires some help to detect that the service
is ready to accept requests. To do that, override the
pytest_userver.plugins.service.service_non_http_health_checks :

@snippet samples/tcp_service/tests/conftest.py  service_non_http_health_checker

## Full sources

See the full example at:
* @ref samples/tcp_service/tcp_service.cpp
* @ref samples/tcp_service/static_config.yaml
* @ref samples/tcp_service/CMakeLists.txt
* @ref samples/tcp_service/tests/conftest.py
* @ref samples/tcp_service/tests/test_tcp.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/production_service.md | @ref scripts/docs/en/userver/tutorial/tcp_full.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/tcp_service/tcp_service.cpp
@example samples/tcp_service/static_config.yaml
@example samples/tcp_service/CMakeLists.txt
@example samples/tcp_service/tests/conftest.py
@example samples/tcp_service/tests/test_tcp.py

