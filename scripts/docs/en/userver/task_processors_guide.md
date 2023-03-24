# Guide on TaskProcessor Usage

engine::TaskProcessor or task processor is a thread pool on which the tasks
(engine::Task, engine::TaskWithresult) are executed.


## Creation

Task processors are configured via the static config file and are created at
the start of the @ref md_en_userver_component_system "component system". Example:

@snippet components/common_component_list_test.cpp  Sample components manager config component config

Any amount of task processors could be created with any names.

## How to use

utils::Async and engine::AsyncNoSpan start a new task on a provided as a
first argument task processor. If the task processor is not provided
utils::Async and engine::AsyncNoSpan use the task processor that
runs the current task (engine::current_task::GetTaskProcessor()). 

A task processor could be obtained from components::ComponentContext in the
constructor of the component. References to task processors outlive the
component system tear-down, they are safe to use from within any components:

@snippet components/component_sample_test.cpp  Sample user component source

@warning If a blocking system call (for example, one that reads
a file in a synchronous way) runs on `main-task-processor`, then the thread
and the processor core are idle until the system call ends. As a result, the
throughput of the service temporarily decreases. To prevent this from
happening, use @ref md_en_userver_intro "userver provided primitives" or
if the primitive is missing, run the blocking system call on a separate task
processor.

Task processors intentionally hide their internals and member functions, so
there's no way to call any of the task processor members directly.

## Common Task Processors

In static configuration we use different names for task processors. The name
does not affect the task processor behavior, only gives a hint on its usage
for the developer.


### main-task-processor

This is usually the default task processor for CPU bound tasks. It is used to
* start the component system, call constructors of the components;
* for initialization of the caches (if not specified otherwise);
* for accepting incoming requests and processing them;
* do all the other CPU bound things;

@note congestion_control::Component at the moment monitors only the
task processor that starts the component system. Handlers that run on other
tasks processors are not controlled.


### fs-task-processor

Task processor for blocking calls. For example, for functions from
`blocking` namespaces, low-level blocking system calls or third-party library
code that does blocking calls.

@note Functions from the userver framework that are not in the
`blocking` namespaces or do not have `blocking` in their name are non-blocking!
For example, there's no need to run
engine::io::Socket::ReadAll() in `fs-task-processor`, use the
`main-task-processor` instead.

A common usage pattern for this task processor looks like:

```cpp
// lib_sample synchronously reads some of /etc/* files.
auto result = engine::AsyncNoSpan(fs_task_processor_, [preset_name]() {
  return lib_sample::quick_check_config_preset(preset_name);
}).Get();
```

Or:
```cpp
auto result = engine::Async(fs_task_processor_, "torrent/validate", [path]() {
  auto files = libtorrent::discover_files(path);  // searches FS
  return libtorrent::validate_hashes(files);      // reads files and computes hashes
}).Get();
```


### single-threaded-task-processors

Used for running task that do not permit concurrent execution
(for example V8 or other interpreters).
components::SingleThreadedTaskProcessors usually
starts those task processors.


### monitor-task-processor

This task processor is used for diagnostic and administration handlers and
tasks. Separate task processor helps to control service under
heavy load or get info from a server with deadlocked threads
in the `main-task-processor`, e.g. server::handlers::InspectRequests handler.


## Moving CPU-bound tasks to a separate task processor

@warning Test and load-test your service, the feature may do things worse.

Some background tasks can slow down handles even if those tasks don't execute
a blocking wait:

* Multiple background tasks can start running at the same time
* Worse if those background tasks are CPU-heavy and don't call engine::Yield
* It can also be a problem if parallel tasks are launched in caches. Then one
  cache is able to fill the entire task processor with tasks

As a workaround for the issue the heavy background tasks could be moved to a
separate `bg-task-processor` task processor.

There are multiple ways to prevent background tasks from consuming 100% of
CPU cores:
* Allocate the minimum possible number of threads for background task
  processors, less than CPU cores.
* Experiment with the `os-scheduling` static option to give lower priority for
  background tasks.

Make sure that tasks execute faster than they arrive.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_logging | @ref md_en_userver_testing ⇨
@htmlonly </div> @endhtmlonly
