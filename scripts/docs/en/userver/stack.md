# Coroutine stack

userver uses coroutines to operate.
A coroutine has its own system stack, a stack per coroutine.
The stack size is 256KB by default.
This is enough for most users, but sometimes larger stack is required to operate.
The stack size is set via `coro_pool.stack_size` option of `components::ManagerControllerComponent` in static config:

```yaml
components_manager:
    coro_pool:
        stack_size: 1000000  # ~1MB
```

## Stack usage

Stack overflow is an issue for any program written in *any* framework/language.
It is a very tricky case to debug, so userver has matters for proactive stack overflow detection.
It can detect and log coroutine stacktrace in case of NN% stack usage.
`engine.coro-pool.stack-usage.max-usage-percent` metric stores the maximum stack usage percent during the task processor lifespan.
If you see this metric approaches values close to 100% then it's time to either reduce stack usage or increase stack size in config.
Userver uses guard pages to protect against stack overflows, but it is not a complete solution.
If you ignore the metric you might face a disaster anyway.
In case of stack overflow the process gets a SIGSEGV on attempt to access missing memory page or even writes garbage to a random memory page.
So you might want to use a metric to avoid the UB.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/congestion_control.md |
@ref scripts/docs/en/userver/caches.md ⇨
@htmlonly </div> @endhtmlonly
