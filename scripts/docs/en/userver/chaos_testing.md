# Chaos Testing

@warning This is the early version of functionality. API may change, issues in chaos.TcpGate may be encountered.

A system could not achieve high availability if it does not tolerate failures. In
microservices and other distributed network architectures the hardware and
network itself is the source of failures.

Luckily userver provides a network proxy to simulate network errors and whose
behavior could be controlled from
@ref md_en_userver_functional_testing "functional tests". In other
words, it is a tool to do chaos testing.

To implement the tests the `chaos.TcpGate` is placed between client and
server. After that, the proxy could be used to test the client and the server
fault tolerance. The following network conditions could be simulated: 

| Network condition                                   | Possible reasons for the behavior in production                 |
|-----------------------------------------------------|-----------------------------------------------------------------|
| Server doesn't accept new connections               | Server is overloaded; network misconfigured                     |
| Client/server does not read data from socket        | Client/server is overloaded; deadlock in client/server thread; OS lost the data on socket due to overload |
| Client/server reads data, but does not respond      | Client/server is overloaded; deadlock in client/server thread   |
| Client/server closes connections                    | Client/server restart; client/server overloaded and shutdowns connections |
| Client/server closes the socket when receives data  | Client/server congestion control in action                      |
| Client/server corrupts the response data            | Stack overflow or other memory corruption at client/server side |
| Network works with send/receive delays              | Poorly performing network; interference in the network          |
| Limited bandwidth                                   | Poorly performing network;                                      |
| Connections close on timeout                        | Timeout on client/server/network interactions from client/server provider |
| Data slicing                                        | Interference in the network; poor routers configuration         |


## chaos.TcpGate

Kepp in mind the following picture:

```
  Client <-> (client) chaos.TcpGate (server) <-> Server
```

`chaos.TcpGate` allows you to simulate different network conditions on `client`
and `server` parts independently. For example, if you wish to corrupt data from
Client to Server, you should use `gate.to_server_corrupt_data()`. If you wish
to corrupt data that goes from Server to Client, you should use
`gate.to_client_corrupt_data()`.

To restore the non failure behavior use `gate.to_*_pass()`.

All the `chaos.TcpGate` functions could be found at
@ref testsuite/pytest_plugins/pytest_userver/chaos.py


## Usage Sample

To use `chaos.TcpGate` it should be imported first:

```python
from pytest_userver import chaos
```

The proxy usually should be started before the service and should keep working
between the service tests runs. To achieve that make a fixture with the session
scope and add it to the service fixture dependencies.

An example how to setup the proxy to stand between the service and PostgreSQL
database:

@snippet postgresql/functional_tests/basic_chaos/tests/conftest.py  gate start

The proxy should be reset to the default state between the test runs, to the
state when connections are accepted and the data pass from client to server and
from server to client:

@snippet postgresql/functional_tests/basic_chaos/tests/conftest.py  gate fixture

After that, everything is ready to write chaos tests:

@snippet postgresql/functional_tests/basic_chaos/tests/test_postgres.py  test cc

Note that the most important part of the test is to make sure that the service
restores functionality after the failures. Wait for connections to proxy make
and sure that the service responses are fine:

@snippet postgresql/functional_tests/basic_chaos/tests/test_postgres.py  restore

### Full Sources

* @ref postgresql/functional_tests/basic_chaos/tests/test_postgres.py
* @ref postgresql/functional_tests/basic_chaos/tests/conftest.py
* @ref postgresql/functional_tests/basic_chaos/postgres_service.cpp
* @ref postgresql/functional_tests/basic_chaos/static_config.yaml
* @ref postgresql/functional_tests/basic_chaos/schemas/postgresql/key_value.sql
* @ref postgresql/functional_tests/basic_chaos/CMakeLists.txt
* @ref testsuite/pytest_plugins/pytest_userver/chaos.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_functional_testing | @ref md_en_userver_profile_context_switches ⇨
@htmlonly </div> @endhtmlonly

@example postgresql/functional_tests/basic_chaos/tests/conftest.py
@example postgresql/functional_tests/basic_chaos/tests/test_postgres.py
@example postgresql/functional_tests/basic_chaos/postgres_service.cpp
@example postgresql/functional_tests/basic_chaos/static_config.yaml
@example postgresql/functional_tests/basic_chaos/schemas/postgresql/key_value.sql
@example postgresql/functional_tests/basic_chaos/CMakeLists.txt
@example testsuite/pytest_plugins/pytest_userver/chaos.py
