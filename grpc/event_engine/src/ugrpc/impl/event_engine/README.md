# EventEngine listener contract

This backend is compiled in Arcadia Linux builds. `PosixEventEngineOwner` owns
and drains the shared server engine independently of the component system. The
factory API is provided by a Linux-only `ya.make` target. Enabling the backend in an
unsupported server build fails an invariant with a message explaining how to
disable the option: it aborts in Debug and throws `utils::InvariantError` in Release.
The backend and the callback API are disabled by default.
To use this backend, explicitly enable both `event-engine.enabled` and `use-callback-api`.

Private core, libev and gRPC headers are confined to the `grpc/event_engine`
library. Other modules use `<userver/ugrpc/impl/event_engine.hpp>` to create
the engine and obtain the public gRPC engine interface from its owner.
`SetEventEngine` attaches the engine to a gRPC server builder. The owner retains
responsibility for shutdown even when other shared references exist. Its
private member of the internal `GrpcInit` type configures the runtime and registers
the channel argument once per process. It then keeps gRPC initialized until the owner is destroyed.
Create the first owner or `GrpcInit` before gRPC initialization or experiment loading; gRPC
rejects late configuration with an assertion. Runtime setup is local to
`grpc_init.cpp`. The dedicated `grpc/event_engine/tests` binary holds its own `GrpcInit`
for the engine tests. Server integration tests belong to `grpc/tests`, whose parent
process keeps the standard gRPC runtime. EventEngine server scenarios run in fresh
child processes so that runtime configuration precedes the first gRPC use.
Creating the owner also fails an invariant unless gRPC has
selected the configured `epoll1` I/O poller, so `GRPC_POLL_STRATEGY` must not
select another one. Construct and shut down the owner in a coroutine while its
task processor and EV threads are still running. Stop dependent servers and
listeners before shutting down the owner.
The embedding runtime must ignore `SIGPIPE`, as the userver component runtime
does for socket I/O.

`BindWithFd`, `Start` and `ShutdownListeningFds` are serialized by the caller.
`BindWithFd` never suspends: a Unix socket path is unlinked and bound on the
calling thread.
The bind callback must not call back into the listener. It receives only bound
descriptors; a failed bind is reported by the returned status alone. A bind
rejected after start or shutdown leaves the previous callback unchanged.

The listen backlog is read from `/proc/sys/net/core/somaxconn` at bind time;
`SOMAXCONN` is used when the file is unavailable.

The listener owns its acceptors through intrusive pointers. Shutdown transfers
the acceptor list into a temporary list, which keeps each acceptor alive while
its shutdown operation runs. Readiness callbacks and deferred operations hold
their own references.

An EMFILE retry timer owns its acceptor until execution or cancellation. When
engine shutdown cancels the timer, destruction of the callback capture releases
that reference without requiring the callback to run. The final shutdown
callback follows destruction of the public listener and its last acceptor.
