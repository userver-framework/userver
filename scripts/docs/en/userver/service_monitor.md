# Service Statistics and Metrics (Prometheus/Graphite/...)

If your service has a server::handlers::ServerMonitor configured, then you may
get the service statistics and metrics.


## Commands

server::handlers::ServerMonitor has a REST API. Full description could be found
at server::handlers::ServerMonitor.

The simplest way to experiment with metrics is to start a sample by running
`make start-userver-samples-production_service` from the build directory and
make some requests from another terminal window, for example
`curl 'http://localhost:8086/service/monitor?format=prometheus'`.

Note that the server::handlers::ServerMonitor handler lives at the separate
`components.server.listener-monitor` address, so you have to request them using the
`listener-monitor` credentials. See @ref md_en_userver_tutorial_production_service
for more info on configuration and ideas on how to change the
`/service/monitor` handle path.

@note `prefix` and `path` parameters may refuse to work if one of the functions
  from userver/utils/statistics/metadata.hpp was used on a node that forms the
  path.


## Formats

Popular metrics formats are supported, like Prometheus or Graphite. Feel free
to fill a feature request or make a PR if some your favorite format is missing.

To specify the format use `format` URL parameter.


## Examples:


### Prometheus metrics by prefix

Prefixes are matched against the metric name:
```
bash
$ curl 'http://localhost:8086/service/monitor?format=prometheus-untyped&prefix=dns'
```
```
dns_client_replies{dns_reply_source="file"} 0
dns_client_replies{dns_reply_source="cached"} 0
dns_client_replies{dns_reply_source="cached-stale"} 0
dns_client_replies{dns_reply_source="cached-failure"} 0
dns_client_replies{dns_reply_source="network"} 0
dns_client_replies{dns_reply_source="network-failure"} 0
``` 


### Graphite metrics by path

Path should math the whole metric name:
```
bash
$ curl 'http://localhost:8086/service/monitor?format=graphite&path=engine.load-ms'
```
```
engine.load-ms 160 1665765043
```


### Get all the metrics in Graphite format

The amount of metrics depends on components count, threads count,
utils::statistics::MetricTag usage and configuration options.
```
bash
$ curl http://localhost:8086/service/monitor?format=graphite | sort
```

@include core/functional_tests/metrics/tests/static/metrics_values.txt


With components::Postgres, some components::PostgreCache and some
storages::postgres::DistLockComponentBase the following additional metrics
appear:

@include postgresql/functional_tests/metrics/tests/static/metrics_values.txt


With components::Mongo and some storages::mongo::DistLockComponentBase the
following additional metrics appear:

@include mongo/functional_tests/metrics/tests/static/metrics_values.txt


With components::Redis the following additional metrics appear:

@include redis/functional_tests/metrics/tests/static/metrics_values.txt


With components::ClickHouse the following additional metrics appear:

@include clickhouse/functional_tests/metrics/tests/static/metrics_values.txt


With components::RabbitMQ the following additional metrics appear:

@include rabbitmq/functional_tests/metrics/tests/static/metrics_values.txt

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_requests_in_flight | @ref md_en_userver_memory_profile_running_service ⇨
@htmlonly </div> @endhtmlonly

