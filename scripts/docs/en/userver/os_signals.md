# Handling OS signals

## Default behavior

- `SIGINT` - return trace
- `SIGTERM`, `SIGQUIT` - shutdown service
- `SIGUSR1` - reopen file descriptors for log (logrotate)


## User behavior

The component os_signals::ProcessorComponent stores os_signals::Processor
and listens to `SIGUSR1` and `SIGUSR2` signals.
You can use `AddListener` function to subscribe to these signals.

For example, we use signal `SIGUSR1` in components::Logging for reopening files
after executing [logrotate](https://github.com/logrotate/logrotate).


### Example

1. Add in private field class

```cpp
os_signals::Subscriber signal_subscribe_
```

2. In in the component's constructor, register a signal listener. For example,
components::Logging uses the following code:

@snippet core/src/logging/component.cpp  Signals sample - init

3. In the component's destructor, unsubscribe from the signal

@snippet core/src/logging/component.cpp  Signals sample - destr


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_dns_control | @ref md_en_userver_caches ⇨
@htmlonly </div> @endhtmlonly
