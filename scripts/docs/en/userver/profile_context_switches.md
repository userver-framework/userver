# Profiling context switches 

This type of profiling is used to detect the most common cases of context
switches. Some use cases for this profiler are:

* the service waits for something. Profiling would help to detect the most
  common cases of waiting for IO, tasks or synchronization primitives.
* the service spends a lot of CPU time under low load. Profiling would help
  to detect the place where the context switching often happens.


## How to profile a service

1. Make sure that service is build with debug information and that the 
cmake option `USERVER_FEATURE_STACKTRACE` was turned `ON`.
2. In the static config file set the `task-trace` options for the task processor
you are willing to profile. All the `task-trace` options are described at
"Static task_processor options" in components::ManagerControllerComponent. It 
is recommended to place traces into a separate
logger to avoid bloat of your default log file. For example:
@snippet components/minimal_server_component_list_test.cpp  Sample task-switch tracing
3. Start your service
4. Look at the traces.


## What to search in profiles

@note Here and below `./scripts/human_logs.py` is used to get more compact logs.

Take a look at the following log:

```
bash
$ make -j4 userver-core_unittest && ./userver/core/userver-core_unittest --gtest_filter=CommonComponentList.ServerMinimal 2>/dev/null \
    | ../scripts/human_logs.py -ign module
[100%] Built target userver-core_unittest
1   Note: Google Test filter = CommonComponentList.ServerMinimal
2   [==========] Running 1 test from 1 test suite.
3   [----------] Global test environment set-up.
4   [----------] 1 test from CommonComponentList
5   [ RUN      ] CommonComponentList.ServerMinimal
6   INFO  Task 7F3081833C00 changed state to kSuspended, delay = 73us 
7   INFO  Task 7F3081833600 changed state to kRunning, delay = 273us 
8   INFO  Task 7F3081833C00 changed state to kQueued, delay = 2736194us 
9   INFO  Task 7F3081833C00 changed state to kRunning, delay = 29us 
10  INFO  Task 7F3081833600 changed state to kCompleted, delay = 334us 
11  INFO  Task 7F308201F500 changed state to kQueued, delay = 0us 
12  INFO  Task 7F308201F500 changed state to kRunning, delay = 57us 
13  INFO  Task 7F3081833C00 changed state to kSuspended, delay = 906us 
14  INFO  Task 7F3081800000 changed state to kRunning, delay = 273us 
15  INFO  Task 7F3081833C00 changed state to kQueued, delay = 54us 
16  INFO  Task 7F3081833C00 changed state to kRunning, delay = 29us 
17  INFO  Task 7F3081833C00 changed state to kSuspended, delay = 56us 
18  INFO  Task 7F3081800000 changed state to kCompleted, delay = 334us 
```

There are two suspicious places:
* At line 8 changing state from `kSuspended` to `kQueued` took too much time.
  Either the task `7F3081833C00` was waiting for some IO or synchronization
  from other task;
  or the task processor is starving on CPU and is not able to process all the
  tasks ASAP.
* Task `7F3081833C00` falls asleep and wakes up quite too often

To investigate the issue further find the suspicious task by its id in the full
logs of the service.

You may also set the level of the
profiling logger to `debug` or `trace` to get the profiles with stacktraces:

```
bash
$ make -j4 userver-core_unittest && ./userver/core/userver-core_unittest --gtest_filter=CommonComponentList.ServerMinimal 2>/dev/null \
    | ../scripts/human_logs.py -ign module -v DEBUG
[100%] Built target userver-core_unittest
1   Note: Google Test filter = CommonComponentList.ServerMinimal
2   [==========] Running 1 test from 1 test suite.
3   [----------] Global test environment set-up.
4   [----------] 1 test from CommonComponentList
5   [ RUN      ] CommonComponentList.ServerMinimal
6   INFO  Task 7F3081833C00 changed state to kQueued, delay = 101us span_id=e806a3e2857714b3	stacktrace= 0# userver::logging::impl::ExtendLogExtraWithStacktrace(userver::logging::LogExtra&, userver::utils::Flags<userver::logging::impl::LogExtraStacktraceFlags>) at /userver/core/src/logging/log_extra_stacktrace.cpp:41
 1# userver::engine::impl::(anonymous namespace)::StacktraceFromLoggerLevel(std::shared_ptr<userver::logging::impl::LoggerWithInfo> const&) at /userver/core/src/engine/task/task_context.cpp:73
 2# userver::engine::impl::TaskContext::TraceStateTransition(userver::engine::Task::State) at /userver/core/src/engine/task/task_context.cpp:729
 3# userver::engine::impl::TaskContext::Schedule() at /userver/core/src/engine/task/task_context.cpp:672
 4# userver::engine::impl::TaskContext::Wakeup(userver::engine::impl::TaskContext::WakeupSource, userver::engine::impl::TaskContext::NoEpoch) at /userver/core/src/engine/task/task_context.cpp:502
 5# userver::engine::impl::WaitList::WakeupOne(userver::engine::impl::WaitList::Lock&) at /userver/core/src/engine/impl/wait_list.cpp:68
 6# userver::engine::Mutex::unlock() at /userver/core/src/engine/mutex.cpp:81
 7# std::lock_guard<userver::engine::Mutex>::~lock_guard() at /usr/include/c++/bits/std_mutex.h:165
 8# userver::concurrent::LockedPtr<std::lock_guard<userver::engine::Mutex>, userver::components::ComponentContext::Impl::ProtectedData const>::~LockedPtr() at /userver/core/include/userver/concurrent/variable.hpp:16
  ... 
7   INFO  Task 7F3081833C00 changed state to kRunning, delay = 584080us span_id=388dad66e4efd590	stacktrace= 0# userver::logging::impl::ExtendLogExtraWithStacktrace(userver::logging::LogExtra&, userver::utils::Flags<userver::logging::impl::LogExtraStacktraceFlags>) at /userver/core/src/logging/log_extra_stacktrace.cpp:41
 1# userver::engine::impl::(anonymous namespace)::StacktraceFromLoggerLevel(std::shared_ptr<userver::logging::impl::LoggerWithInfo> const&) at /userver/core/src/engine/task/task_context.cpp:73
 2# userver::engine::impl::TaskContext::TraceStateTransition(userver::engine::Task::State) at /userver/core/src/engine/task/task_context.cpp:729
 3# userver::engine::impl::TaskContext::Sleep(userver::engine::impl::WaitStrategy&) at /userver/core/src/engine/task/task_context.cpp:368
 4# userver::engine::Mutex::LockSlowPath(userver::engine::impl::TaskContext&, userver::engine::Deadline) at /userver/core/src/engine/mutex.cpp:63
 5# userver::engine::Mutex::try_lock_until(userver::engine::Deadline) at /userver/core/src/engine/mutex.cpp:92
 6# userver::engine::Mutex::lock() at /userver/core/src/engine/mutex.cpp:73
 7# std::lock_guard<userver::engine::Mutex>::lock_guard(userver::engine::Mutex&) at /usr/include/c++/bits/std_mutex.h:159
 8# userver::concurrent::LockedPtr<std::lock_guard<userver::engine::Mutex>, userver::components::ComponentContext::Impl::ProtectedData const>::LockedPtr(userver::engine::Mutex&, userver::components::ComponentContext::Impl::ProtectedData const&) at /userver/core/include/userver/concurrent/variable.hpp:20
 ...
```


## FAQ

- **Q:** I get barely readable traces, without function or file names.

  **A:** You need to make sure that the service was build with the
  cmake option `USERVER_FEATURE_STACKTRACE` turned `ON`. Also check that
  the debug information was not stripped away from the service and that you
  have a modern `libbacktrace` library on your system.
  
- **Q:** Can I use it on a service running in production?

  **A:** Yes, but note that stacktrace demangling may add unpredictable
  slowdowns and do disk IO (due to reading debug symbols of a binary from disk).


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_functional_testing | @ref md_en_userver_grpc ⇨
@htmlonly </div> @endhtmlonly
