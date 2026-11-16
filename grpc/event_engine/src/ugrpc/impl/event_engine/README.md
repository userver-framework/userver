# EventEngine listener contract

This backend is compiled in Arcadia Linux builds. `PosixEventEngineOwner` owns
and drains the shared server engine independently of the component system.

Private core, libev and gRPC headers are confined to the `grpc/event_engine`
library. Other modules use `<userver/ugrpc/impl/event_engine.hpp>` to initialize
the runtime and obtain the public gRPC engine interface from its owner.
`SetEventEngine` attaches the engine to a gRPC server builder. The owner retains
responsibility for shutdown even when other shared references exist. Call
`EnsureRuntimeConfigured()` and `RegisterEventEngineArgument()` explicitly before
`grpc_init()` or constructing gRPC objects. Retain a gRPC initialization reference
until owner shutdown completes. Construct and shut down the owner in
a coroutine while its task processor and EV threads are still running. Stop
dependent servers and listeners before shutting down the owner.
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
