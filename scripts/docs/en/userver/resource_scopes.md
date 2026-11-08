# Resource scopes

Resource scopes tie resources such as subscriptions and background tasks to an object's lifetime. The object's scopes
are stored in @ref utils::ResourceScopeStorage.

A callback registered directly in a constructor may run before the object is fully initialized. Unregistering in the
destructor can also be too late: the callback may access state that is already being destroyed. Scopes provide hooks
after construction and before destruction. APIs use these hooks to start and stop access to the object.

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

The helper must own the data accessed by its callback. Do not move registration into a separate object that only holds
references to that data: the wrapper controls that object's lifetime, but does not protect the referenced data from
destruction.

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

