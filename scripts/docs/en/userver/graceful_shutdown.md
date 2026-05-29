## Graceful shutdown

By default, when a user-based service receives a SIGTERM or SIGINT signal, it swiftly closes all active connections and halts all components.

However, in certain scenarios, a service might need to shut down gracefully. This means it should inform its clients of the impending shutdown, provide them with extra time to submit any pending requests, and attempt to complete all ongoing operations. This process is known as a graceful shutdown.

Graceful shutdown is deactivated by default. It can be activated by setting the `graceful_shutdown_continue_accepting_requests_interval` and `graceful_shutdown_pending_requests_completion_interval` parameters in the configuration of the components::ManagerControllerComponent.

\b static_config.yaml example:
@snippet core/functional_tests/graceful_shutdown/static_config.yaml graceful_shutdown_settings

Note: The previous `graceful_shutdown_interval` parameter has been deprecated and should be migrated to the new parameters. Currently, it serves as a fallback for new parameters when they are not specified. However, it will be completely removed in the future.

If one or both parameters are non-zero a service will switch to *graceful shutdown* mode after SIGTERM or SIGINT.
It corresponds to @ref components::ServiceLifetimeStage::kGracefulShutdown service lifetime stage.

At this stage the service will:

* Start failing HTTP and gRPC health checks:
  * HTTP @ref scripts/docs/en/userver/tutorial/production_service.md "/ping" handler will return 500 errors.
  * gRPC `grpc.health.v1.Health` service will return `NOT_SERVING` status.
* Append @ref graceful_shutdown_headers "special headers" to HTTP responses and gRPC response metadata,
  if enabled.
* If `graceful_shutdown_continue_accepting_requests_interval` is non-zero, accept new HTTP and gRPC requests and continue processing of ongoing requests for the given time interval (10 seconds in the example above).
* Once the previous stage completed (or disabled) and if `graceful_shutdown_pending_requests_completion_interval` is non-zero, stop accepting new HTTP connections and gRPC requests and continue processing of ongoing requests for the given time interval (5 seconds in the example above).
Note: Current HTTP server implementation may continue to accept new requests on already established connections. But, you should not rely on this behavior.

@anchor graceful_shutdown_headers
### Graceful shutdown headers

During a graceful shutdown, a service appends special HTTP headers and gRPC metadata to outgoing responses, provided the feature is enabled. Typically, the initial metadata is employed for gRPC. However, if a graceful shutdown is triggered after the initial metadata has already been sent, trailing metadata will be utilized instead.

Graceful shutdown headers are enabled by default. They could be configured or disabled via @ref GRACEFUL_SHUTDOWN_HEADERS dynamic configuration.

@example grpc/functional_tests/middleware_server/tests-graceful-shutdown-first-stage/test_first_stage.py


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/long_transactions.md | @ref scripts/docs/en/userver/caches.md ⇨
@htmlonly </div> @endhtmlonly
