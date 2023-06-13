# Deploy Environment Specific Configurations

This page describes typical setups of the service that uses userver framework.
The purpose of the page is not to provide an ideal solution with perfectly matching
3rd-party tools, but rather show different approaches and give a starting
point for designing and configuring an environment.


### All-in-one single instance setup

A simple setup, usually used during development. The request comes directly
into the service, it is processed, logs are written. During processing the
service could do direct requests to other services. 

@dot
digraph TypicalService
{
  node [shape=component];
  compound=true;
  fixedsize=true;
  rankdir=LR;
  tooltip = "You didn't hit the arrow with the cursor :-)";
  labeljust = "l";
  labelloc = "t";

  subgraph cluster_TypicalService {
    shape=component;
    label = "Docker container or host machine";
    center=true;
    
    Service [label = "🐙 Service", shape=component, width=2.0 ];
    logs [label = "File with\nservice logs", shape=note, width=2.0];
    Database [label = "Database", shape=cylinder, width=2.0 ];
  }

  OtherService [label = "Some Other Service", shape=component, width=2.0];
  OtherService1 [label = "Another Service", shape=component, width=2.0];

  Request -> Service[penwidth=3, edgetooltip = "TCP, UDP, HTTP or gRPC", minlen=0, dir=both arrowtail = none];
  Database -> Service  [penwidth=3, edgetooltip = "binary protocol", dir=both arrowhead = none];
  logs -> Service  [penwidth=3, edgetooltip = "FS write", dir=both  arrowhead=none];
  Service -> OtherService [penwidth=3, edgetooltip = "TCP, UDP, HTTP, HTTP2 or gRPC", dir=both  arrowtail=none];
  Service -> OtherService1 [penwidth=3, edgetooltip = "TCP, UDP, HTTP, HTTP2 or gRPC", dir=both  arrowtail=none];
 
  Request[label = "Request", shape=plaintext, rank="main"];
}
@enddot

**Pros:**
* Simple

**Cons:**
* Is not reliable, only a single service instance exists.
* Database affects efficiency and reliability of the service
* Logs and metrics could be retrieved only by direct inspecting of the container


This configuration is quite useful for testing. However, there's no need to
configure it manually for tests, because testsuite does that automatically
(starts databases, fills them with data, tunes logging, mocks other services).
See @ref md_en_userver_functional_testing for more info.

For non testing purposes the configuration could be also quite useful for
services that should have a single instance and have small load on database.
Internal services that require no reliability and chat bots are examples of
such services. For a starting point on configuration see
@ref md_en_userver_tutorial_production_service.

Note that for a longstanding runs the logs of the service should be cleaned up
at some point. Configure the `logrotate` like software to move/remove the old
logs and notify the `🐙 Service` by `SIGUSR1` signal:  
```
postrotate
    /usr/bin/killall -USR1 yourapp
```


### Production setup with balancer

A good setup for a production. The request comes into a service balancer,
balancer routes the request into one of the instances of the setup. In the instance
the request goes into the Nginx (or some other reverse proxy, or just goes
directly to the `🐙 Service`). Nginx could serve static requests, terminate TLS,
do some header rewrites and forward request to the `🐙 Service`.

The service uses
@ref md_en_userver_tutorial_config_service "dynamic configs service", writes
logs.

Some `Metrics Uploader` script is periodically called, it
@ref md_en_userver_service_monitor "retrieves metrics"
from the `🐙 Service` and sends the results to `Metrics Aggregateor` (could be
Prometheus, Graphite, VictoriaMetrics). Metrics could be viewed in web
interface, for example via Grafana .

`Logs Collector` (for example `logstash`, `fluentd`, `vector`) processes the
logs file and uploads results to logs aggregator
(for example Elastic). Logs could be viewed in web interface, for
example via Kibana.

@dot
digraph TypicalService
{
  node [shape=component];
  compound=true;
  fixedsize=true;
  rankdir=LR;
  tooltip = "You didn't hit the arrow with the cursor :-)";
  labeljust = "l";
  labelloc = "t";

  ServiceBalancer[label = "Balancers", shape=octagon, width=1.4]

  subgraph cluster_TypicalService {
    shape=component;
    label = "Docker container";
    center=true;
    
    Nginx [label = "Nginx", shape=component, width=2.0 ];
    
    Service [label = "🐙 Service", shape=component, width=2.0 ];
    logs [label = "File with\nservice logs", shape=note, width=2.0];
    LogsCollector [label = "Logs Collector", shape=component, width=2.0];
    MetricsUploader[label = "Metrics Uploader", shape=component, width=2.0];
  }
  
  subgraph cluster_ServiceDb {
    label = "DB Cluster";
    
    Database [label = "Database", shape=cylinder, width=2.0 ];
    Database1 [label = "Database", shape=cylinder, width=2.0];
  }
  
  Elastic [shape=cylinder, width=2.0];
  Kibana [shape=component, width=2.0];
  
  MetricsAggregateor [label = "Metrics Aggregateor", shape=cylinder, width=2.0];
  Grafana [shape=component, width=2.0];
  
  BalancerConfigsService [label = "Balancers", shape=octagon, width=2.0];
  ConfigsService [label = "Configs Service", shape=component, width=2.0];

  BalancerOtherService [label = "Balancers", shape=octagon, width=2.0];
  BalancerOtherService1 [label = "Balancers", shape=octagon, width=2.0];

  OtherService [label = "Some Other Service", shape=component, width=2.0];
  OtherService1 [label = "Another Service", shape=component, width=2.0];

  Request -> ServiceBalancer[penwidth=3, edgetooltip = "TCP socket, compressed HTTP", minlen=0, dir=both arrowtail = none];
  ServiceBalancer -> Nginx [penwidth=3, edgetooltip = "TCP socket, compressed HTTP, or gRPC",  dir=both arrowtail = none];
  
  Nginx -> Service [penwidth=3, edgetooltip = "Local Pipe, uncompressed HTTP", labelfloat=true];
  logs -> Service  [penwidth=3, edgetooltip = "FS write, TSKV", dir=both  arrowhead=none];
  Database1 -> Service  [penwidth=3, edgetooltip = "binary protocol", dir=both arrowhead = none];
  Service -> BalancerConfigsService [penwidth=3, edgetooltip = "HTTP JSON, compressed response", weight=100];
  BalancerConfigsService -> ConfigsService [penwidth=3, edgetooltip = "HTTP JSON, compressed response", weight=100];
  Service -> BalancerOtherService [penwidth=3, edgetooltip = "HTTP JSON, compressed response", dir=both  arrowtail=none];
  Service -> BalancerOtherService1 [penwidth=3, edgetooltip = "HTTP JSON, compressed response", dir=both  arrowtail=none];
  BalancerOtherService -> OtherService [penwidth=3, edgetooltip = "HTTP JSON, compressed response", dir=both  arrowtail=none];
  BalancerOtherService1 -> OtherService1 [penwidth=3, edgetooltip = "HTTP JSON, compressed response", dir=both  arrowtail=none];
  logs -> LogsCollector  [penwidth=3, edgetooltip = "FS read, TSKV", dir=both  arrowhead=none];
  MetricsUploader -> Service [penwidth=3, edgetooltip = "HTTP JSON, compressed response"];
  MetricsUploader -> MetricsAggregateor[penwidth=3, edgetooltip = "HTTP JSON, compressed response"];
  LogsCollector -> Elastic [penwidth=3, edgetooltip = "HTTP JSON, compressed response", weight=100];
  Elastic -> Kibana [penwidth=3, edgetooltip = "JSON", dir=both];
  MetricsAggregateor -> Grafana [penwidth=3, edgetooltip = "JSON", dir=both];

  Request[label = "Request", shape=plaintext, rank="main"];

  /* Fake edges for alignment */
  Database1 -> Database [style = invis];
}
@enddot

**Pros:**
* Reliable - multiple instances of service, balancers and database exist.
* Service and database do not affect each other.
* Diagnosable - logs and metrics from all the instances are available
  for the developers.


**Cons:**
* Hard to reconfigure balancers, especially on instance count change
* Balancers require separate containers


`Metrics Uploader` script could be implemented in bash or Python to request
@ref md_en_userver_service_monitor "service monitor". Note that for
small containers the script could eat up quite a lot of resources if it
converts metrics from one format to another. Prefer using already supported
metrics format and feel free to add new formats via Pull Requests
to [userver github](https://github.com/userver-framework/userver).

To avoid TCP/IP overhead it is useful to configure the `🐙 Service` to
interact with `Nginx` via pipes. For HTTP this could be done by configuring the
components::Server to use `unix-socket` rather than `port` static configuration
option.

Beware of the `Logs Collector` software. Some of those applications are not
very reliable and could eat up a lot of dynamic memory. In general, it is a
good idea to limit their memory consumption by cgroups like mechanism or
adjust OOM-killer priorities to kill the `Logs Collector` rather that the
`🐙 Service` itself. Also, do not forget to configure `logrotate`, as was
shown in the first recipe.

See @ref md_en_userver_tutorial_production_service for more configuration
tips and tricks.


### Production setup with service-mesh

A good production setup that tries to solve issues with balancers
from previous setup.

The request comes into a `Sidecar Proxy` (like Envoy) directly.
It could do some work of routing, header rewrites and forwards request
to the `🐙 Service`.


`Sidecar Proxy` is configured via `xDS` (service discovery service) and knows
about all the instances of all the services. If the `🐙 Service` has to do a
network request, it does it via `Sidecar Proxy`,
that routes the request directly into one of the instances of the service. See
@ref USERVER_HTTP_PROXY "USERVER_HTTP_PROXY" for an info on how to route all
the HTTP client requests to the `Sidecar Proxy`.

All other parts of the setup remain the same as in the previous approach.

The service uses
@ref md_en_userver_tutorial_config_service "dynamic configs service", writes
logs.

Some `Metrics Uploader` script is periodically called, it
@ref md_en_userver_service_monitor "retrieves metrics"
from the `🐙 Service` and sends the results to `Metrics Aggregateor`.
Metrics could be viewed in web interface, for example via Grafana .

`Logs Collector` (for example `logstash`, `fluentd`, `vector`) processes the
logs file and uploads results to logs aggregator
(for example Elastic). Logs could be viewed in web interface, for
example via Kibana.

@dot
digraph TypicalService
{
  node [shape=component];
  compound=true;
  fixedsize=true;
  rankdir=LR;
  tooltip = "You didn't hit the arrow with the cursor :-)";
  labeljust = "l";
  labelloc = "t";

  subgraph cluster_TypicalService {
    shape=component;
    label = "Docker container";
    center=true;
    
    SidecarProxy [label = "Sidecar Proxy", shape=octagon, width=2.0 ];
    
    Service [label = "🐙 Service", shape=component, width=2.0 ];
    logs [label = "File with\nservice logs", shape=note, width=2.0];
    LogsCollector [label = "Logs Collector", shape=component, width=2.0];
    MetricsUploader[label = "Metrics Uploader", shape=component, width=2.0];
  }
  
  subgraph cluster_ServiceDb {
    label = "DB Cluster";
    
    Database [label = "Database", shape=cylinder, width=2.0 ];
    Database1 [label = "Database", shape=cylinder, width=2.0];
  }
  
  Elastic [shape=cylinder, width=2.0];
  Kibana [shape=component, width=2.0];
  
  MetricsAggregateor [shape=cylinder, width=2.0];
  Grafana [shape=component, width=2.0];
  xDS [shape=component, width=2.0];
  
  ConfigsService [label = "Configs Service", shape=component, width=2.0];
  
  OtherService [label = "Some Other Service", shape=component, width=2.0];
  OtherService1 [label = "Another Service", shape=component, width=2.0];

  Request -> SidecarProxy[penwidth=3, edgetooltip = "TCP socket, compressed HTTP, or gRPC", minlen=0, dir=both arrowtail = none];
  SidecarProxy -> Service [penwidth=3, edgetooltip = "Local Pipe, uncompressed HTTP, or gRPC", labelfloat=true];
  logs -> Service  [penwidth=3, edgetooltip = "FS write, TSKV", dir=both  arrowhead=none];
  Database1 -> Service  [penwidth=3, edgetooltip = "binary protocol", dir=both arrowhead = none];
  Service -> SidecarProxy  [penwidth=3, edgetooltip = "HTTP, HTTPS, compressed response"];
  xDS -> SidecarProxy [penwidth=3, edgetooltip = "gRPC polling", arrowhead=none, arrowtail=diamond, weight=100];
  SidecarProxy -> ConfigsService [penwidth=3, edgetooltip = "HTTP JSON, compressed response", weight=100];
  SidecarProxy -> OtherService [penwidth=3, edgetooltip = "HTTP JSON, compressed response", dir=both  arrowtail=none];
  SidecarProxy -> OtherService1 [penwidth=3, edgetooltip = "HTTP JSON, compressed response", dir=both  arrowtail=none];
  logs -> LogsCollector  [penwidth=3, edgetooltip = "FS read, TSKV", dir=both  arrowhead=none];
  MetricsUploader -> Service [penwidth=3, edgetooltip = "HTTP JSON, compressed response"];
  MetricsUploader -> MetricsAggregateor[penwidth=3, edgetooltip = "HTTP JSON, compressed response"];
  LogsCollector -> Elastic [penwidth=3, edgetooltip = "HTTP JSON, compressed response", weight=100];
  Elastic -> Kibana [penwidth=3, edgetooltip = "JSON", dir=both];
  MetricsAggregateor -> Grafana [penwidth=3, edgetooltip = "JSON", dir=both];

  
  Request[label = "Request", shape=plaintext, rank="main"];

  /* Fake edges for alignment */
  Database1 -> Database [style = invis];
}
@enddot

**Pros:**
* Service and database do not affect each other.
* Diagnosable - logs and metrics from all the instances are available for
  the developers.
* Less network hops and simple configuration of balancing


**Cons:**
* `xDS` becomes a single point of failure if the `Sidecar Proxy` could not start
  without it after container restart.

The setup is very close to the setup from previous recipe (except for the
@ref USERVER_HTTP_PROXY "USERVER_HTTP_PROXY"). refer to it for more tips.


### Sidecar

This setup is useful for implementing some supplementary functionality to the
main service. Usually the same sidecar is used for multiple different
services of different logic.

Otherwise, the sidecar could be used as a first step of decomposing the
main service into smaller parts.

@dot
digraph TypicalService
{
  node [shape=component];
  compound=true;
  fixedsize=true;
  rankdir=TB;
  tooltip = "You didn't hit the arrow with the cursor :-)";
  labeljust = "l";
  labelloc = "t";

  subgraph cluster_TypicalService1 {
    shape=component;
    label = "Docker container";
    center=true;
    
    MainService1 [label = "Main Service", shape=component, width=2.0 ];    
    Sidecar1 [label = "🐙 Sidecar", shape=component, width=2.0 ];
  }
  
  subgraph cluster_TypicalService2 {
    shape=component;
    label = "Docker container";
    center=true;
    
    MainService2 [label = "Other Service", shape=component, width=2.0 ];    
    Sidecar2 [label = "🐙 Sidecar", shape=component, width=2.0 ];
  }
  
  subgraph cluster_TypicalService3 {
    shape=component;
    label = "Docker container";
    center=true;
    
    MainService3 [label = "Another Service", shape=component, width=2.0 ];    
    Sidecar3 [label = "🐙 Sidecar", shape=component, width=2.0 ];
  }

  MainService1 -> Sidecar1[penwidth=3, edgetooltip = "pipeline, TCP socket, compressed HTTP, or gRPC", minlen=0, dir=both];
  MainService2 -> Sidecar2[penwidth=3, edgetooltip = "pipeline, TCP socket, compressed HTTP, or gRPC", minlen=0, dir=both];
  MainService3 -> Sidecar3[penwidth=3, edgetooltip = "pipeline, TCP socket, compressed HTTP, or gRPC", minlen=0, dir=both];
}
@enddot

**Pros:**
* Increases reliability of the main service by moving out supplementary logic
* Could be reused for different applications

In cases, where the sidecar is used to decompose main service to smaller parts
the tips from previous recipes apply.

For the case of supplementary logic the sidecar usually does not do heavy
logic and should be configured for minimal resource consumption. Remove the
unused components from code and/or adjust the
static config file:
```
# yaml
components_manager:
    coro_pool:
        initial_size: 100         # Save memory and do not allocate many coroutines at start.
        max_size: 200             # Do not keep more than 200 preallocated coroutines.

    task_processors:
        main-task-processor:
            worker_threads: 1     # Process tasks in 1 thread.

    default_task_processor: main-task-processor

    # A sidecar doesn't need to trade RES memory for responsiveness in rare edge cases
    mlock_debug_info: false

components:
      congestion-control:
          load-enabled: false

      # A sidecar doesn't want detailed HTTP client statistics
      http-client:
          destination-metrics-auto-max-size: 0
          pool-statistics-disable: true
          threads: 1

      # Sidecars should not upload system statistics
      system-statistics-collector:
          load-enabled: false

      # Sidecars usually do not provide business logic HTTP handlers
      handler-implicit-http-options:
          load-enabled: false
      handler-inspect-requests:
          load-enabled: false
```

Also, there's usually no need in very high responsiveness of the sidecar,
so use the same `main-task-processor` for all the task processors.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_tutorial_build | @ref md_en_userver_beta_state ⇨
@htmlonly </div> @endhtmlonly
