# Dump coroutines in runtime

Sometimes your service may experience some bugs (wow!).
It may result in subtle behaviour including deadlocks and coroutines freeze.
If you faced similar issue in production, you might want to carefully study some/all running coroutines properties to locate and fix the bug.

## Setup

Collecting coroutines data is rather costly and might significantly slow down your service.
That's why it is disabled by default and should be explicitly enabled.
To do that, please do the following:

1) In your static config set `trace-coroutines: true` for task processor:

```
yaml

components_manager:
    ...
    task_processors:
        main-task-processor:
            trace-coroutines: true
```

It turns on runtime hooks for coroutines lifetime events, including creation/destruction/awaits/etc.

2) Register dump coroutines handler into `main`:

```
cpp
#include <userver/server/handlers/dump_coroutines.hpp>

int main(int argc, const char* const argv[]) {
    const auto component_list = components::ComponentList()
        ...
        .Append<server::handlers::DumpCoroutines>();
}
```

3) Add handler settings into the static config:

```
yaml
components_manager:
    components:
        ...
        handler-dump-coroutines:
            path: /service/dump-coroutines
            method: GET
            task_processor: monitor-task-processor
```

4) Start your service and do some HTTP/GRPC requests. Please be careful!
The service with coroutine tracer is VERY slow, benchmarks show up to 0.5ms slowdown on context switch.
Too high load might overload the process.

5) Make a HTTP request to `service/dump-coroutines` handler:

```
bash
$ curl 'http://127.0.0.1:1188/service/dump-coroutines'
Or even:
$ curl 'http://127.0.0.1:1188/service/dump-coroutines' | json_pp | sed 's/\\n/\n\t/g'
```

The command returns a long list of active coroutines, their properties including stacktrace.
You might want to store the response in a file and then study the contents.

An example content:

```
json
[
   {
      "link" : "90cbf4bfb9d4a52d78639e2897d2569f",
      "span" : "system_statistics_collector",
      "stacktrace" : " 0# engine::TracePlugin::HookBeforeSleep(engine::impl::TaskContext const&) at /home/segoon/arcadia3/taxi/uservices/userver/core/src/engine/tracer_plugin.cpp:33
	 1# engine::PluginManager::HookBeforeSleep(engine::impl::TaskContext const&) at /home/segoon/arcadia3/taxi/uservices/userver/core/src/engine/plugin_manager.hpp:74
	 2# engine::TaskProcessor::HookBeforeSleep(engine::impl::TaskContext const&) at /home/segoon/arcadia3/taxi/uservices/userver/core/src/engine/task/task_processor.cpp:327
	 3# engine::impl::TaskContext::Sleep(engine::impl::WaitStrategy&, engine::Deadline) at /home/segoon/arcadia3/taxi/uservices/userver/core/src/engine/task/task_context.cpp:333
	 4# engine::SingleConsumerEvent::WaitForEventUntil(engine::Deadline) at /home/segoon/arcadia3/taxi/uservices/userver/core/src/engine/single_consumer_event.cpp:54
	 5# bool engine::SingleConsumerEvent::WaitForEventUntil<std::__y1::chrono::steady_clock, std::__y1::chrono::duration<long long, std::__y1::ratio<1l, 1000000000l>>>(std::__y1::chrono::time_point<std::__y1::chrono::steady_clock, std::__y1::chrono::duration<long long, std::__y1::ratio<1l, 1000000000l>>>) at /-S/taxi/uservices/userver/core/include/userver/engine/single_consumer_event.hpp:119
	 6# utils::PeriodicTask::Impl::Run() at /-S/taxi/uservices/userver/core/src/utils/periodic_task.cpp:202
	 7# decltype(*std::declval<utils::PeriodicTask::Impl*>().*std::declval<void (utils::PeriodicTask::Impl::*)()>()()) std::__y1::__invoke[abi:fe200000]<void (utils::PeriodicTask::Impl::*)(), utils::PeriodicTask::Impl*, void>(void (utils::PeriodicTask::Impl::*&&)(), utils::PeriodicTask::Impl*&&) at /-S/contrib/libs/cxxsupp/libcxx/include/__type_traits/invoke.h:117
	 8# decltype(auto) std::__y1::__apply_tuple_impl[abi:fe200000]<void (utils::PeriodicTask::Impl::*)(), std::__y1::tuple<utils::PeriodicTask::Impl*>, 0ul>(void (utils::PeriodicTask::Impl::*&&)(), std::__y1::tuple<utils::PeriodicTask::Impl*>&&, std::__y1::__tuple_indices<0ul>) at /-S/contrib/libs/cxxsupp/libcxx/include/tuple:1365
	 9# decltype(auto) std::__y1::apply[abi:fe200000]<void (utils::PeriodicTask::Impl::*)(), std::__y1::tuple<utils::PeriodicTask::Impl*>>(void (utils::PeriodicTask::Impl::*&&)(), std::__y1::tuple<utils::PeriodicTask::Impl*>&&) at /-S/contrib/libs/cxxsupp/libcxx/include/tuple:1369
	10# utils::impl::WrappedCallImpl<void (utils::PeriodicTask::Impl::*)(), utils::PeriodicTask::Impl*>::Perform() at /-S/taxi/uservices/userver/core/include/userver/utils/impl/wrapped_call.hpp:104
	11# engine::impl::TaskContext::CoroFunc(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&) at /home/segoon/arcadia3/taxi/uservices/userver/core/src/engine/task/task_context.cpp:529
	12# boost::coroutines2::detail::push_coroutine<engine::impl::TaskContext*>::control_block::control_block<engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&)>(boost::context::preallocated, engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&))::'lambda'(boost::context::fiber&&)::operator()(boost::context::fiber&&) at /-S/contrib/restricted/boost/coroutine2/include/boost/coroutine2/detail/push_control_block_cc.ipp:90
	13# decltype(std::declval<engine::coro::debug::MarkedAllocator&>()(std::declval<boost::context::fiber>())) std::__y1::__invoke[abi:fe200000]<boost::coroutines2::detail::push_coroutine<engine::impl::TaskContext*>::control_block::control_block<engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&)>(boost::context::preallocated, engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&))::'lambda'(boost::context::fiber&&)&, boost::context::fiber>(engine::coro::debug::MarkedAllocator&, boost::context::fiber&&) at /-S/contrib/libs/cxxsupp/libcxx/include/__type_traits/invoke.h:149
	14# std::__y1::invoke_result<engine::coro::debug::MarkedAllocator&, boost::context::fiber>::type std::__y1::invoke[abi:fe200000]<boost::coroutines2::detail::push_coroutine<engine::impl::TaskContext*>::control_block::control_block<engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&)>(boost::context::preallocated, engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&))::'lambda'(boost::context::fiber&&)&, boost::context::fiber>(engine::coro::debug::MarkedAllocator&, boost::context::fiber&&) at /-S/contrib/libs/cxxsupp/libcxx/include/__functional/invoke.h:29
	15# boost::context::detail::fiber_record<boost::context::fiber, engine::coro::debug::MarkedAllocator&, boost::coroutines2::detail::push_coroutine<engine::impl::TaskContext*>::control_block::control_block<engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&)>(boost::context::preallocated, engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&))::'lambda'(boost::context::fiber&&)>::run(void*) at /-S/contrib/restricted/boost/context/include/boost/context/fiber_fcontext.hpp:206
	16# void boost::context::detail::fiber_entry<boost::context::detail::fiber_record<boost::context::fiber, engine::coro::debug::MarkedAllocator&, boost::coroutines2::detail::push_coroutine<engine::impl::TaskContext*>::control_block::control_block<engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&)>(boost::context::preallocated, engine::coro::debug::MarkedAllocator&, void (* const&)(boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>&))::'lambda'(boost::context::fiber&&)>>(boost::context::detail::transfer_t) at /-S/contrib/restricted/boost/context/include/boost/context/fiber_fcontext.hpp:146
	",
      "task" : true
   }
]
```

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/stack.md | @ref scripts/docs/en/userver/long_transactions.md ⇨
@htmlonly </div> @endhtmlonly
