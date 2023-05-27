## Production configs and best practices

A good production ready service should have functionality for various cases:
* Overload
  * Service should respond with HTTP 429 codes to some requests while still being able to handle the rest
* Debugging of a running service
  * inspect logs
  * get more logs from the suspiciously behaving service and then turn the logging level back
  * profile memory usage
  * see requests in flight
* Experiments
  * Should be a way to turn on/off arbitrary functionality without restarting the service
* Metrics and Logs
* Functional testing

This tutorial shows a configuration of a typical production ready service. For
information about service interactions with other utilities and services in
container see @ref md_en_userver_deploy_env.


## Before you start

Make sure that you can compile and run core tests and read a basic example @ref md_en_userver_tutorial_hello_service.

## int main

utils::DaemonMain initializes and starts the component system with the provided command line arguments:

@snippet samples/production_service/production_service.cpp Production service sample - main

A path to the static config file should be passed from a command line to start the service:

```
bash
./samples/userver-samples-production_service --config /etc/production-service/static_config.yaml
```

## Static config

Full static config could be seen at @ref samples/production_service/static_config.yaml

Important parts are described down below.

### Variables

Static configs tend to become quite big, so it is a good idea to move changing parts of it into variables. To do that,
declare a `config_vars` field in the static config and point it to a file with variables.

@snippet samples/production_service/static_config.yaml Production service sample - static config - config vars

A file with config variables could look @ref samples/production_service/config_vars.yaml "like this". 

Now in static config you could use `$variable-name` to refer to a variable, 
`*#fallback` fields are used if there is no variable with such name in the config variables file:

@snippet samples/production_service/static_config.yaml Production service sample - static config variables usage


### Task processors

A good practice is to have at least 3 different task processors:

@snippet samples/production_service/static_config.yaml Production service sample - static config task processors

Moving blocking operations into a separate task processor improves responsiveness and CPU usage of your service. Monitor task processor
helps to get statistics and diagnostics from server under heavy load or from a server with a deadlocked threads in the main task processor.

@warning This setup is for an abstract service on an abstract 8 core machine. Benchmark your service on your hardware and hand-tune the
thread numbers to get optimal performance.


### Listeners/Monitors

Note the components::Server configuration:

@snippet samples/production_service/static_config.yaml Production service sample - static config server listen

In this example we have two listeners. it is done to separate clients and utility/diagnostic handlers to listen on different ports or even interfaces.


@anchor sample_prod_service_utility_handlers
### Utility handlers

Your server has the following utility handlers:
* to @ref md_en_userver_requests_in_flight "inspect in-flight request" - server::handlers::InspectRequests
* to @ref md_en_userver_memory_profile_running_service "profile memory usage" - server::handlers::Jemalloc
* to @ref md_en_userver_log_level_running_service "change logging level at runtime" - server::handlers::LogLevel
  and server::handlers::DynamicDebugLog
* to reopen log files after log rotation (you can also use @ref md_en_userver_os_signals "signals") - server::handlers::OnLogRotate 
* to @ref md_en_userver_dns_control "control the DNS resolver" - server::handlers::DnsClientControl
* to @ref md_en_userver_service_monitor "get statistics" from the service - server::handlers::ServerMonitor

@snippet samples/production_service/static_config.yaml Production service sample - static config utility handlers

All those handlers live on a separate `components.server.listener-monitor`
address, so you have to request them using the `listener-monitor` credentials:

```
bash
$ curl http://localhost:8085/service/log-level/
{"init-log-level":"info","current-log-level":"info"}

$ curl -X PUT 'http://localhost:8085/service/log-level/warning'
{"init-log-level":"info","current-log-level":"warning"}
```


### Ping

This is a server::handlers::Ping handle that returns 200 if the service is OK, 500 otherwise. Useful for balancers,
that would stop sending traffic to the server if it responds with codes other than 200.

@snippet samples/production_service/static_config.yaml Production service sample - static config ping

Note that the ping handler lives on the task processor of all the other handlers. Smart balancers may measure response times and send less traffic to the heavy loaded services. 

```
bash
$ curl --unix-socket service.socket http://localhost/ping -i
HTTP/1.1 200 OK
Date: Thu, 01 Jul 2021 12:46:07 UTC
Content-Type: text/html; charset=utf-8
X-YaRequestId: 39e3f54b86984b8ca5235876dc566b27
Server: sample-production-service 1.0
X-YaTraceId: 4d7f8aa03e2d4e4d80a92a3ccecfbe6d
Connection: keep-alive
Content-Length: 0

```


### Dynamic configs

Here's a configuration of a dynamic config related components
components::DynamicConfigClient, components::DynamicConfig,
components::DynamicConfigClientUpdater.

Service starts with dynamic config values from `dynamic-config.fs-cache-path` file
or from `dynamic-config-client-updater.fallback-path` file. Service updates dynamic
values from a @ref md_en_userver_tutorial_config_service "configs service".

@snippet samples/production_service/static_config.yaml Production service sample - static config dynamic configs


### Congestion Control

congestion_control::Component limits the active requests count. In case of overload it responds with HTTP 429 codes to some requests, allowing your service to properly process handle the rest.

All the significant parts of the component are configured by dynamic config options @ref USERVER_RPS_CCONTROL and @ref USERVER_RPS_CCONTROL_ENABLED 

@snippet samples/production_service/static_config.yaml Production service sample - static config congestion-control

It is a good idea to disable it in unit tests to avoid getting HTTP 429 on an overloaded CI server.


@anchor tutorial_metrics
### Metrics

Metrics is a convenient way to monitor the health of your service.

Typical setup of components::SystemStatisticsCollector and components::StatisticsStorage is quite trivial:

@snippet samples/production_service/static_config.yaml Production service sample - static config metrics

With such setup you could poll the metrics from handler server::handlers::ServerMonitor that we've configured in @ref sample_prod_service_utility_handlers "previous section". However
a much more mature approach is to write a component that pushes the metrics directly into the remote metrics aggregation service or
to write a handle that provides the metrics in the native aggregation service format.


### Secdist - secrets distributor

Storing sensitive data aside from the configs is a good practice that allows you to set different access rights for the two files.

components::Secdist configuration is straightforward:

@snippet samples/production_service/static_config.yaml Production service sample - static config secdist

Refer to the storages::secdist::SecdistConfig config for more information on the data retrieval. 


@anchor prod_service_testsuite_related_components
### Testsuite related components

server::handlers::TestsControl is a handle that allows controlling the service
from test environments. That handle is used by the testsuite from
@ref md_en_userver_functional_testing "functional tests" to mock time,
invalidate caches, testpoints and many other things. This component should be
disabled in production environments.

components::TestsuiteSupport is a lightweight storage to keep minor testsuite
data. This component is required by many high-level components and it is safe to
use this component in production environments.


## Dynamic config

Initial values of the dynamic config could be seen at @ref samples/production_service/dynamic_config_fallback.json

Those are described in details at @ref md_en_schemas_dynamic_configs .

### Build

This sample requires @ref md_en_userver_tutorial_config_service "configs service", so we build and start one from our previous tutorials.

```
bash
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..

make userver-samples-config_service
./samples/userver-samples-config_service &

make userver-samples-production_service
python3 ../samples/tests/prepare_production_configs.py
./samples/userver-samples-production_service --config /tmp/userver/production_service/static_config.yaml
```


### Functional testing
@ref md_en_userver_functional_testing "Functional tests" are used to make sure
that the service is working fine and
implements the required functionality. A recommended practice is to build the
service in Debug and Release modes and tests both of them, then deploy the
Release build to the production, @ref "disabling all the tests related handlers".

Debug builds of the userver provide numerous assertions that validate the
framework usage and help to detect bugs at early stages.

Typical functional tests for a service consist of a `conftest.py` file with
mocks+configs for the sereffectivelyvice and a bunch of `test_*.py` files with actual
tests. Such approach allows to reuse mocks and configurations in different
tests.

## Full sources

See the full example at 
* @ref samples/production_service/production_service.cpp
* @ref samples/production_service/static_config.yaml
* @ref samples/production_service/config_vars.yaml
* @ref samples/production_service/dynamic_config_fallback.json
* @ref samples/production_service/CMakeLists.txt
* @ref samples/production_service/tests/conftest.py
* @ref samples/production_service/tests/test_ping.py
* @ref samples/production_service/tests/test_production.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_tutorial_config_service | @ref md_en_userver_tutorial_tcp_service ⇨
@htmlonly </div> @endhtmlonly

@example samples/production_service/production_service.cpp
@example samples/production_service/static_config.yaml
@example samples/production_service/config_vars.yaml
@example samples/production_service/CMakeLists.txt
@example samples/production_service/dynamic_config_fallback.json
@example samples/production_service/tests/conftest.py
@example samples/production_service/tests/test_ping.py
@example samples/production_service/tests/test_production.py
