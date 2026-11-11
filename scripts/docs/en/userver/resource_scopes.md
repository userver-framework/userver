# Resource scopes

Resource scopes tie resources such as subscriptions and background tasks to an object's lifetime. The object's scopes
are stored in @ref utils::ResourceScopeStorage.

## APIs that use scopes

Pass scopes to the API that manages the resource:

- Metrics: @ref utils::statistics::Storage::RegisterWriter or @ref utils::statistics::RegisterWriterScope.
  See @ref metrics_register_writer "registering a metrics callback".
- Dynamic config updates: @ref dynamic_config::Source::UpdateAndListen.
  See @ref dynamic_config_usage "using dynamic config".
- Cache updates: @ref components::CachingComponentBase::UpdateAndListen.
  See @ref scripts/docs/en/userver/caches.md.
- Cache resets in testsuite: @ref testsuite::RegisterCacheResetter.
  See @ref testsuite::CacheControl and @ref scripts/docs/en/userver/functional_testing.md.

The scoped `UpdateAndListen` overloads invoke the listener synchronously with the current value. Scopes defer subsequent
notifications until construction is complete and unsubscribe before destruction. The initial synchronous call must be
safe during construction. See each API's contract for details.


@anchor resource_scopes_register
## Using Register directly

Call @ref utils::ResourceScopeStorage::Register directly to manage a resource that has no scoped API. Pass a function
that starts the resource and returns a RAII handle that stops it. The function runs after construction, and the scopes
destroy the handle before the object's destructor starts.

For example, a base class can use this to start an asynchronous task or a @ref utils::PeriodicTask after the derived
object is fully constructed, and stop it before the derived object's destructor starts:

- For an asynchronous task, return the @ref engine::TaskWithResult created by @ref utils::Async from the registration
  function. Destroying this handle cancels the task and waits for its completion.
- For a @ref utils::PeriodicTask, start the task in the registration function and return a @ref utils::FastScopeGuard
  that calls @ref utils::PeriodicTask::Stop. This stops the task and waits for any running callback to finish.
  See @ref scripts/docs/en/userver/periodics.md for choosing and testing periodic tasks.


@anchor resource_scopes_parameter
## Where to obtain scopes

### Component scopes

If the callback calls a method of the component itself, use `context.Scopes()` in the component constructor.

The component system opens the scopes after all component constructors, including derived constructors, have
completed. It closes them before the component's destructor starts, while the component's data is still available.

Scopes manage registration lifetime, but do not synchronize access to data. If a callback runs concurrently with
other code, access to shared data must be thread-safe.

### Scopes for other objects

A helper class that is not a component should accept a @ref utils::ResourceScopeStorage reference as the first
constructor argument. Pass this reference to the APIs that register its callbacks or subscriptions.

Choose scopes according to when the helper is constructed and destroyed:

- A helper created during component construction and owned by that component until its destruction can receive
  `context.Scopes()` as its first constructor argument. One example is an `Impl` object holding the component's
  implementation. Another is a @ref clients_from_components_lifetime "client" owned by the component and exposed to
  other components by reference.
- A helper created after component construction or destroyed before the component needs its own scopes. For example,
  this applies to objects that a factory component creates while the service is running.

Passing a factory component's scopes to the helpers it creates ties their registrations to the component's lifetime.
If a helper is destroyed earlier, its callback remains registered and can access the destroyed helper.


#### Choosing a wrapper

For a helper that needs its own scopes, do not construct @ref utils::ResourceScopeStorage manually, including in tests.
Choose a wrapper according to ownership:

- @ref utils::MakeUniqueWithResourceScopes returns a `std::unique_ptr<T>` for unique ownership. `T` must not be `final`
  and must have a public virtual destructor.
- @ref utils::MakeWithResourceScopes returns a `std::shared_ptr<T>` when shared ownership is needed.
- @ref utils::WithResourceScopes stores `T` by value inside the wrapper. Pass constructor arguments after
  `std::in_place`.

All three wrappers pass a @ref utils::ResourceScopeStorage reference as the first constructor argument. They run
registration callbacks after the helper is fully constructed and unregister subscriptions before its destructor starts.

For example, the wrapped object owns data that its subscription accesses:

@snippet core/src/components/resource_scopes_test.cpp MakeWithResourceScopes


#### Data ownership

Choose the registering object according to the subscription's lifetime:

- The subscription must last for the lifetime of an existing object. Wrap that object in resource scopes.
  Register the subscription in its constructor. Do not move registration into a separate object that only references
  the existing object's state.
- The subscription must end before the main object is destroyed. Create a separate object with its own scopes.
  It may reference the main object's data. The referenced data must outlive the subscription object.

Do not make @ref utils::ResourceScopeStorage a member of the helper itself: the member's destructor is called from the
helper's destructor, when the helper's state may already be partially destroyed. Use one of the wrappers above to
unregister the callback before calling the helper's destructor.


## Scope order

Normally, data accessed by callbacks is stored in the object's members. If other scopes initialize and destroy that
data, register the callback's scope after them. This makes the data available before registration and unregisters the
callback before the data is destroyed.

For example, a @ref dynamic_config::Source::UpdateAndListen listener initializes a field synchronously, then keeps its
value up to date for the subscription's lifetime. If a metrics callback reads that field, register the metrics after
calling `UpdateAndListen`. This initializes the field before the metrics callback is registered and unregisters that
callback before removing the config subscription.


## Problems solved by resource scopes

A callback registered directly in a constructor may run before the object is fully initialized. Unregistering in a
destructor can also be too late: the callback may access state that is already being destroyed. Scopes provide hooks
after construction and before destruction to keep registration within the lifetime of the data it uses.

The event-subscription example uses `WeatherStorage` and the weather types from the
@ref concurrent::AsyncEventChannel example.

### Managing subscription and data lifetimes

Members are destroyed in reverse declaration order. If a subscription handle is declared before the data its callback
uses, that data is destroyed first. The callback can still run until the handle unsubscribes.

The following illustrative pseudocode assumes a hypothetical `UpdateAndListen` API returning a RAII
`SubscriptionHandle`. It shows the incorrect member order:

```cpp
class WeatherHistory {
public:
    explicit WeatherHistory(WeatherStorage& weather_storage) {
        subscription_ = weather_storage.UpdateAndListen(this, "history", &WeatherHistory::OnWeatherUpdate);
    }

private:
    void OnWeatherUpdate(WeatherKind weather) { history_.push_back(weather); }

    SubscriptionHandle subscription_;
    std::vector<WeatherKind> history_;
};
```

For this hypothetical API, declaring the handle last makes it unsubscribe before the other members are destroyed.
Another option is to call `SubscriptionHandle::Unsubscribe()` explicitly in the destructor body, while those members
are still alive. Both approaches require keeping the handle's lifetime in the correct order as the class changes.

A handle declared last in a base class only protects that base class's members. If the callback uses members of a
derived class, those members are already destroyed when the base destructor runs.

Component scopes and the scope wrappers close the subscription before the most-derived destructor starts.
Unsubscribing waits for running callbacks while the object's members, including derived members, are still available.

In this example, the callback updates a member of the object:

@snippet core/src/concurrent/async_event_channel_test.cpp Resource scopes subscription state

Member declaration order therefore does not affect when the subscription is removed.

### Calling virtual methods from callbacks

A metrics callback may run in a parallel collector. Registering it from a base constructor can expose the object before
the derived constructor finishes. If the callback calls a virtual method, it can reach a pure virtual function or
access derived state that has not been initialized.

The following incorrect illustrative pseudocode assumes a hypothetical `RegisterWriter` overload without scopes. The
current API does not provide this overload; `SubscriptionHandle` denotes its imagined RAII handle:

```cpp
class MetricsBase {
public:
    explicit MetricsBase(utils::statistics::Storage& storage) {
        subscription_ = storage.RegisterWriter("worker", [this](utils::statistics::Writer& writer) {
            writer["value"] = GetValue();
        });
    }

    virtual ~MetricsBase() = default;

private:
    virtual std::int64_t GetValue() const = 0;

    SubscriptionHandle subscription_;
};
```

A concurrent collector can invoke the callback while the derived object is being constructed or destroyed.
Unregistering in the base destructor is too late: the derived members have already been destroyed, and a concurrent
callback can still access them before the base handle unregisters.

The scoped @ref utils::statistics::Storage::RegisterWriter overload defers the entire registration until construction
is complete. The derived object owns the metric's value:

@snippet core/src/utils/statistics/storage_test.cpp Resource scopes virtual callback

The component system or wrapper opens the scopes after the most-derived constructor finishes, so a collector's
virtual call reaches a valid derived object. Scopes close and wait for running callbacks before the most-derived
destructor starts, while the derived state is still available.

### Pausing notifications until construction completes

Consider a subscriber to an external event source. Immediately after `UpdateAndListen`, its constructor throws:

@snippet core/src/concurrent/async_event_channel_test.cpp Resource scopes constructor failure

After the exception, stack unwinding destroys the subscriber's initialized members. The external source may send
an update concurrently with this destruction. If the subscription delivered such updates, the callback could access
`updates_` while that member is being destroyed or afterwards, until scope cleanup.

Scoped `UpdateAndListen` cannot detect when the constructor body exits with an exception. The destructor of the object
whose construction failed is not called, and the component system or wrapper only cleans up the scopes after the
initialized members have been destroyed.

The scoped `UpdateAndListen` overloads therefore call the initial callback synchronously, then pause further
notifications until the scopes open. Scopes open only after the most-derived constructor succeeds. If events were
sent during this pause, the initial callback is called again with the current state before notifications start.
Without such events, there is no second initialization call.

If construction throws, the scopes never open, so the user callback cannot run again after its initial synchronous
invocation. Pausing notifications thus protects the subscriber's data throughout constructor unwinding.

@note This pause breaks the usual single update frontier of subscriptions through @ref concurrent::AsyncEventChannel:
other subscribers may have already processed an update while the component being constructed still uses the previous
state. A long-running constructor may therefore use outdated configuration or cache values. We chose this tradeoff
because the component has no users until construction completes, and the pause protects its data if construction
throws.
