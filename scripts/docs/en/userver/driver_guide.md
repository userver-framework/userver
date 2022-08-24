## Driver Writing Guide

Do you want to write a new driver for the server? This work is noble, but
difficult.

There are multiple ways to write a driver.


### Easy to write, poor performance

Create a separate `TaskProcessor` for the driver and put all the blocking
interactions of the native library into it:

```cpp
template <class... Args>
auto UserverSide::DoStuff(Args&&... args) { 
  return utils::Async(driver_task_processor, "driver-action", [&]() {
    return native_do_stuff(std::forward<Args>(args));
  }).Get();
}
```

This approach has a problem. userver discourages performing CPU-bound
work outside of the `main-task-processor` and prohibits blocking system calls
in the `main-task-processor`.

While the threads of the `{driver}-task-processor` are blocked on I/O,
there are no problems. But as soon as they start parsing data the invariant
above breaks, which leads lead to degradation of the service.

This approach is discouraged.


### Not too easy to write, good performance

If the native driver provides some kind of notification mechanism for
asynchronous completions, then you can notify userver from the completion
queue using asynchronous server primitives (engine::Future or
engine::SingleConsumerEvent). This approach was used to write the gRPC driver
in userver.

Otherwise, if the native driver is written in C++ and is not too big, then
it may be possible to patch it to use userver IO primitives
(engine::io::Socket, for example) and cleaning out other blocking primitives
in favor of userver ones (engine::Mutex, engine::ConditionVariable). This
approach was used to write the Clickhouse driver in userver.

In both approaches there is a downside. The native library must be integrated
into CMake scripts and additional libraries should be added to userver
dependencies. In some rare cases this might be non-trivial.


### Hard to write, good performance

Implement the protocol yourself (probably reusing a subset of the native
library functionality).  This approach was used to write the PostgreSQL driver
in userver - it creates a
connection in a separate `TaskProcessor` via native functions, and then
subscribes to the socket via engine::io::Socket, parses the protocol in the
main `TaskProcessor`.

Such an approach may open the door for further optimizations. If no native
library is reused, then there's no need in dealing with CMake scripts to integrate it.


## Recommendations

Your driver should be well integrated into the userver framework. Write logs, a
lot of logs, write them in all places where something is happening.
It's always better to have extra logs (which can then be removed) than to have
errors without logs.

Write metrics - about connections, about queries, about query timings - in
general, about everything that happens inside the driver.

Write tests - unit and functional ones. If possible, write
benchmarks. For full-fledged testing, you will need to
set up an instance of what you are writing a driver for in the testsuite.
This may not be trivial...

Docs and samples should be provided.

Note: PR for new drivers are welcome! If you started the work, the driver
already does something useful but you're feeling exhausted, then we may
help you to deal with the remaining parts.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_development_releases | @ref md_en_userver_publications ⇨
@htmlonly </div> @endhtmlonly
