# HTTP, HTTPS, WebSocket

## Introduction

🐙 **userver** implements HTTP/HTTPS 1.1 and WebSocket server in `userver-core` library using @ref components::Server component.

## Capabilities

* HTTP 1.1/1.0 support;
* HTTPS;
* @ref scripts/docs/en/userver/tutorial/websocket_service.md "WebSocket";
* Body decompression with "Content-Encoding: gzip";
* HTTP pipelining;
* Custom authorization @ref scripts/docs/en/userver/tutorial/auth_postgres.md ;
* Rate limiting via Congestion control and indiviadual handlers configuration;
* Requests-in-flight limiting;
* Requests-in-flight inspection via server::handlers::InspectRequests ;
* Body size / headers count / URL length / etc. limits;
* Streaming;
* @ref scripts/docs/en/userver/tutorial/multipart_service.md "File uploads and multipart/form-data"
* @ref scripts/docs/en/userver/deadline_propagation.md .

## Streaming API

Interactive clients (e.g. web browser) might want to get at least first bytes of the HTTP response if the whole HTTP response is generated slowly. In this case the HTTP handler might want to use Streaming API and return HTTP response body as a byte stream rather than as a single-part blob.

To enable Streaming API in your handler:

1) define HandleStreamRequest() method:
```cpp
  #include <userver/server/http/http_response_body_stream_fwd.hpp>
  ...
    void HandleStreamRequest(const server::http::HttpRequest&,
                             server::request::RequestContext&,
                             server::http::ResponseBodyStream&) const override;
```

2) Enable Streaming API in static config:
```yaml
components_manager:
    components:
        handler-stream-api:
            response-body-stream: true
```

3) Set dynamic config @ref USERVER_HANDLER_STREAM_API_ENABLED.

4) Write your handler code:

@snippet core/functional_tests/basic_chaos/httpclient_handlers.hpp HandleStreamRequest

## Components

* @ref components::Server "Server"
