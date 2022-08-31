# Controlling DNS

If your service has a server::handlers::DnsClientControl configured, then you
may force hosts reload from cache and wipe DNS caches. The feature is useful
to apply DNS changes faster.

## Commands
server::handlers::DnsClientControl provides the following REST API:
```
POST /service/dnsclient/reload_hosts
POST /service/dnsclient/flush_cache?name={some name}
POST /service/dnsclient/flush_cache_full
```
Note that the server::handlers::DnsClientControl handler lives at the separate
`components.server.listener-monitor` address, so you have to request them using the
`listener-monitor` credentials. See @ref md_en_userver_tutorial_production_service
for more info on configuration and ideas on how to change the
`/service/dnsclient/` handle path.

## Examples:

### Reload hosts file

```
bash
$ curl -X POST http://localhost:8085/service/dnsclient/reload_hosts -i
HTTP/1.1 200 OK
Date: Thu, 02 Dec 2021 15:27:49 UTC
Content-Type: text/html; charset=utf-8
X-YaRequestId: e952064e60454c19950a534799837479
X-YaSpanId: 2e67da42409d2f2a
X-YaTraceId: 6e45cff5d60b4ca2b399d39b8d093351
Server: sample-production-service 1.0
Connection: keep-alive
Content-Length: 2

OK
```

### Rediscover the ya.ru DNS records
```
bash
$ curl -X POST http://localhost:8085/service/dnsclient/flush_cache?name=ya.ru -i
HTTP/1.1 200 OK
Date: Thu, 02 Dec 2021 15:29:20 UTC
Content-Type: text/html; charset=utf-8
X-YaRequestId: 6bfa28dd4ee74d18ab49d871b78426bf
X-YaSpanId: 6f886740835e81bf
X-YaTraceId: ffc2a2128d4c439891bd3b76ad77f80b
Server: sample-production-service 1.0
Connection: keep-alive
Content-Length: 2

OK
```

### Rediscover all the DNS records
```
bash
$ curl -X POST http://localhost:8085/service/dnsclient/flush_cache_full -i
HTTP/1.1 200 OK
Date: Thu, 02 Dec 2021 15:30:44 UTC
Content-Type: text/html; charset=utf-8
X-YaRequestId: 79fc8ca39ac5475ab7be72ff5b9fecf2
X-YaSpanId: dd8ab87efa690283
X-YaTraceId: 2df06ec8be4a4dbdb66d4af78d4eba83
Server: sample-production-service 1.0
Connection: keep-alive
Content-Length: 2

OK
```


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_memory_profile_running_service | @ref md_en_userver_os_signals ⇨
@htmlonly </div> @endhtmlonly
