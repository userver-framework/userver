# Inspecting in-flight requests

If your service has a server::handlers::InspectRequests configured, then you may
inspect the active requests. The feature is useful for investigating a broken
production service to find request parameters that lead to handle malfunction. 

## Commands
server::handlers::InspectRequests provides the following REST API:
```
GET /service/inspect-requests
GET /service/inspect-requests?body=1
```
Note that the server::handlers::InspectRequests handler lives at the separate
`components.server.listener-monitor` address, so you have to request them using the
`listener-monitor` credentials. See @ref md_en_userver_tutorial_production_service
for more info on configuration and ideas on how to change the
`/service/inspect-requests` handle path.

## Examples:

### Get the requests in flight

In this example there is one in-flight request that is being executed for more
than 3 seconds.

```
bash
$ curl http://localhost:8085/internal/inspect-requests | jq
```
```json
[
  {
    "method": "GET",
    "url": "/hello",
    "http_version": "1.1",
    "request_path": "/hello",
    "handling-duration-ms": 3865,
    "start-handling-timestamp": "Thu, 02 Dec 2021 07:57:22 UTC",
    "host": "localhost",
    "args": {},
    "headers": {
      "User-Agent": "curl/7.58.0",
      "Accept": "*/*",
      "Host": "localhost"
    }
  }
]
```

### Get the bodies of the in-flight requests 
In this example there are multiple in-flight request that actually have no body.
```
bash
$ curl http://localhost:8085/internal/inspect-requests?body=1 | jq
```
```json
[
  {
    "request-body": "",
    "method": "GET",
    "url": "/hello",
    "http_version": "1.1",
    "request_path": "/hello",
    "handling-duration-ms": 11909,
    "start-handling-timestamp": "Thu, 02 Dec 2021 08:11:20 UTC",
    "host": "localhost",
    "args": {},
    "headers": {
      "User-Agent": "curl/7.58.0",
      "Accept": "*/*",
      "Host": "localhost"
    }
  },
  {
    "request-body": "",
    "method": "GET",
    "url": "/hello",
    "http_version": "1.1",
    "request_path": "/hello",
    "handling-duration-ms": 11303,
    "start-handling-timestamp": "Thu, 02 Dec 2021 08:11:21 UTC",
    "host": "localhost",
    "args": {},
    "headers": {
      "User-Agent": "curl/7.58.0",
      "Accept": "*/*",
      "Host": "localhost"
    }
  },
  {
    "request-body": "",
    "method": "GET",
    "url": "/hello",
    "http_version": "1.1",
    "request_path": "/hello",
    "handling-duration-ms": 10938,
    "start-handling-timestamp": "Thu, 02 Dec 2021 08:11:21 UTC",
    "host": "localhost",
    "args": {},
    "headers": {
      "User-Agent": "curl/7.58.0",
      "Accept": "*/*",
      "Host": "localhost"
    }
  }
]
```


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_log_level_running_service | @ref md_en_userver_service_monitor ⇨
@htmlonly </div> @endhtmlonly

