## The Basics

## Restrictions

Usage of `catch (...)` without `throw;` **should be avoided** as the framework
may use exceptions not derived from `std::exception` to manage some resources.
Usage of `catch` with explicit exception type specification (like
`std::exception` or `std::runtime_error`) is fine without `throw;`.

🐙 **userver** uses its own coroutine scheduler, which is unknown to the C++
standard library, as well as to the libc/pthreads. The standard library for
synchronization often uses mutexes, other synchronization primitives and event
waiting mechanisms that block the current thread. When using userver, this
results in the current thread not being able to be used to execute other
coroutines. As a result, the number of threads executing coroutines decreases.
This can lead to a huge performance drops and increased latencies.

For the reasons described above, the use of synchronization primitives or IO
operations of the C++ standard library and libc in the
@ref scripts/docs/en/userver/task_processors_guide.md "main task processor"
**should be avoided** in high-load applications. The same goes for all functions and
classes that use blocking IO operations or synchronization primitives.

**⚠️🐙❗ Instead of the standard primitives, you need to use the primitives from the userver:**

| Standard primitive                | Replacement from userver                                                     |
|-----------------------------------|------------------------------------------------------------------------------|
| `thread_local`                    | @ref userver_thread_local "It depends, but do not use standard thread_local" |
| `std::this_thread::sleep_for()`   | `engine::SleepFor()`                                                         |
| `std::this_thread::sleep_until()` | `engine::SleepUntil()`                                                       |
| `std::mutex`                      | `engine::Mutex`                                                              |
| `std::shared_mutex`               | `engine::SharedMutex`                                                        |
| `std::condition_variable`         | `engine::ConditionVariable`                                                  |
| `std::future<T>`                  | `engine::TaskWithResult<T>` or `engine::Future`                              |
| `std::async()`                    | `utils::Async()`                                                             |
| `std::thread`                     | `utils::Async()`                                                             |
| `std::counting_semaphore`         | `engine::Semaphore`                                                          |
| network sockets                   | `engine::io::Socket`                                                         |
| `std::filesystem::`               | `::fs::*` (but not `::fs::blocking::*`!)                                     |
| `std::cout`                       | `LOG_INFO()`                                                                 |
| `std::cerr`                       | `LOG_WARNING()` and `LOG_ERROR()`                                            |

An overview of the main synchronization mechanisms is available
[on a separate page](scripts/docs/en/userver/synchronization.md).

Note that if your application is not meant for high-load and does not require
low-latency, then it may be fine to run all the code on the same task processor.

______
⚠️🐙❗ If you want to run code that uses standard synchronization primitives
(for example, code from a third-party library), then this code should be run in
a separate `engine::TaskProcessor` to avoid starvation of main task processors.
See @ref scripts/docs/en/userver/task_processors_guide.md for more info.
______


@anchor intro_tasks
## Tasks

The asynchronous **task** (@ref engine::Task, @ref engine::TaskWithResult) can return
a result (possibly in form of an exception) or return nothing. In any case, the
task has the semantics of future, i.e. you can wait for it and get the result
from it.

To create a task call the @ref utils::Async function. It accepts the name of a
task, and the user-defined function to execute:

```cpp
auto task = utils::Async("my_job", [some, &captures] {
    return Use(some, captures);
});
// do something ...
auto result = task.Get();
```

Like `std::async`, you can call an existing function asynchronously, passing it some args:

```cpp
auto task = utils::Async("my_job", SomeFunc, copied_arg, std::move(moved_arg), std::ref(ref_arg));
// do something ...
auto result = task.Get();
```

@anchor flavors_of_async
### Flavors of Async

There are multiple orthogonal parameters of the task being started.
Use this specific overload by default (@ref utils::Async).

By engine::TaskProcessor:

* By default, task processor of the current task is used.
* Custom task processor can be passed as the first or second function
  parameter (see function signatures).

By shared-ness:

* By default, functions return engine::TaskWithResult, which can be awaited
  from 1 task at once. This is a reasonable choice for most cases.
* Functions from `utils::Shared*Async*` and `engine::Shared*AsyncNoSpan`
  families return engine::SharedTaskWithResult, which can be awaited
  from multiple tasks at the same time, at the cost of some overhead.

By engine::TaskBase::Importance ("critical-ness"):

* By default, functions can be cancelled due to engine::TaskProcessor
  overload. Also, if the task is cancelled before being started, it will not
  run at all.
* If the whole service's health (not just one request) depends on the task
  being run, then functions from `utils::*CriticalAsync*` and
  `engine::*CriticalAsyncNoSpan*` families can be used. There, execution of
  the function is guaranteed to start regardless of engine::TaskProcessor
  load limits

By tracing::Span:

* Functions from `utils::*Async*` family (which you should use by default)
  create tracing::Span with inherited `trace_id` and `link`, a new `span_id`
  and the specified `stopwatch_name`, which ensures that logs from the task
  are categorized correctly and will not get lost.
* Functions from `engine::*AsyncNoSpan*` family create span-less tasks:
    * A possible usage scenario is to create a task that will mostly wait
      in the background and do various unrelated work every now and then.
      In this case it might make sense to trace execution of work items,
      but not of the task itself.
    * Its usage can (albeit very rarely) be justified to squeeze some
      nanoseconds of performance where no logging is expected.
      But beware! Using tracing::Span::CurrentSpan() will trigger asserts
      and lead to UB in production.

By the propagation of engine::TaskInheritedVariable instances:

* Functions from `utils::*Async*` family (which you should use by default)
  inherit all task-inherited variables from the parent task.
* Functions from `engine::*AsyncNoSpan*` family do not inherit any
  task-inherited variables.

By deadline: some `utils::*Async*` functions accept an @ref engine::Deadline
parameter. If the deadline expires, the task is cancelled. See `*Async*`
function signatures for details.

### Scoping of tasks

A task is only allowed to run within the lifetime of its @ref engine::TaskWithResult handle.
If the control flow escapes the task definition scope while the task is running, it is
cancelled and awaited in the task's destructor:

```cpp
std::string FrobnicateBoth(std::string_view first, std::string_view second) {
    auto first_task = utils::Async("frobnicate_first", Frobnicate, first);
    auto second_task = utils::Async("frobnicate_second", Frobnicate, second);
    if (SomethingBadHappened()) throw std::runtime_error("Nope");
    return first_task.Get() + second_task.Get();
}
```

If an exception is thrown before the tasks are finished, they will be cancelled and awaited.
In general, those tasks will be awaited anyway and will not keep running in the background indefinitely.

This is the backbone of **structured concurrency** in userver.

To make the task keep running in the background:
@see @ref concurrent::BackgroundTaskStorage

For more details on task cancellations:
@see @ref task_cancellation_intro

### Lifetime of captures

@note To launch background tasks, which are not awaited in the local scope,
use @ref concurrent::BackgroundTaskStorage.

When launching a task, it's important to ensure that it will not access its
lambda captures after they are destroyed. Plain data captured by value
(including by move) is always safe. By-reference captures and objects that
store references inside are always something to be aware of. Of course,
copying the world will degrade performance, so let's see how to ensure
lifetime safety with captured references.

Task objects are automatically cancelled and awaited on destruction, if not
already finished. The lifetime of the task object is what determines when
the task may be running and accessing its captures. The task can only safely
capture by reference objects that outlive the task object.

When the task is just stored in a new local variable and is not moved or
returned from a function, capturing anything is safe:

@code
int x{};
int y{};
// It's recommended to write out captures explicitly when launching tasks.
auto task = utils::Async("frobnicate", [&x, &y] {
// Capturing anything defined before the `task` variable is safe.
Use(x, y);
});
// ...
task.Get();
@endcode

A more complicated example, where the task is moved into a container:

@code
// Variables are destroyed in the reverse definition order: y, tasks, x.
int x{};
std::vector<engine::TaskWithResult<void>> tasks;
int y{};

tasks.push_back(utils::Async("frobnicate", [&x, &y] {
// Capturing x is safe, because `tasks` outlives `x`.
Use(x);

    // BUG! The task may keep running for some time after `y` is destroyed.
    Use(y);
}));
@endcode

The bug above can be fixed by placing the declaration of `tasks` after `y`.

In the case above people often think that calling `.Get()` in appropriate
places solves the problem. It does not! If an exception is thrown somewhere
before `.Get()`, then the variables' definition order is the source
of truth.

Same guidelines apply when tasks are stored in classes or structs: the task
object must be defined below everything that it accesses:

@code
private:
Foo foo_;
// Can access foo_ but not bar_.
engine::TaskWithResult<void> task_;
Bar bar_;
@endcode

Generally, it's a good idea to put task objects as low as possible
in the list of class members.

Although, tasks are rarely stored in classes on practice,
@ref concurrent::BackgroundTaskStorage is typically used for that purpose.

Components and their clients can always be safely captured by reference:

@see @ref scripts/docs/en/userver/component_system.md


## Waiting

The code inside the coroutine may want to wait for an external event - a
response from the database, a response from the HTTP client, the arrival of a
certain time. If a coroutine wants to wait, it tells the engine that it wants
to suspend its execution, and another coroutine starts executing on the current
thread of the operating system instead. As a result, the thread is not idle,
but reused by other users. After an external event occurs, the coroutine
will be scheduled and executed.

```cpp
f();
engine::SleepFor(std::chrono::seconds(60)); // voluntarily giving the current thread to other coroutines
g(); // The thread has returned to us
```

@anchor task_cancellation_intro
## Task cancellation

A task can be notified that it needs to discard its progress and finish early. Once cancelled, the task remains cancelled until its completion. Cancelling a task permanently interrupts most awaiting operations in that task.

### Ways to cancel a task

Cancellation can occur:

  * by an explicit request;
  * due to the end of the task object lifetime;
  * at coroutine engine shutdown (affects tasks launched via `engine::Task::Detach`);
  * due to the lack of resources.

To cancel a task explicitly, call the `engine::TaskBase::RequestCancel()` or `engine::TaskBase::SyncCancel()` method. It cancels only a single task and does not directly affect the subtasks that were created by the canceled task.

Another way to cancel a task it to drop the `engine::TaskWithResult` without awaiting it, e.g. by returning from the function that stored it in a local variable or by letting an exception fly out.

@snippet core/src/engine/task/cancel_test.cpp stack unwinding destroys task

Tasks can be cancelled due to `engine::TaskProcessor` overload, if configured. This is a last-ditch effort to avoid OOM due to a spam of tasks. Read more in `utils::Async` and `engine::TaskBase::Importance`. Tasks started with `engine::CriticalAsync` are excepted from cancellations due to `TaskProcessor` overload.

### How the task sees its cancellation

Unlike C++20 coroutines, userver does not have a magical way to kill a task. The cancellation will somehow be signaled to the synchronization primitive being waited on, then it will go through the logic of the user's function, then the function will somehow complete.

How some synchronization primitives react to cancellations:

  * `engine::TaskWithResult::Get` and `engine::TaskBase::Wait` throw `engine::WaitIterruptedException`, which typically leads to the destruction of the child task during stack unwinding, cancelling and awaiting it;
  * `engine::ConditionVariable::Wait` and `engine::Future::wait` return a status code;
  * `engine::SingleConsumerEvent::WaitForEvent` returns `false`;
  * `engine::SingleConsumerEvent::WaitForEventFor` returns `false` and needs an additional `engine::current_task::ShouldCancel()` check;
  * `engine::InterruptibleSleepFor` needs an additional `engine::current_task::ShouldCancel()` check;
  * `engine::CancellableSemaphore` returns `false` or throws `engine::SemaphoreLockCancelledError`.

Some synchronization primitives deliberately ignore cancellations, notably:

  * `engine::Mutex`;
  * `engine::Semaphore` (use `engine::CancellableSemaphore` to support cancellations);
  * `engine::SleepFor` (use `engine::InterruptibleSleepFor` to support cancellations).

Most clients throw a client-specific exception on cancellation. Please explore the docs of the client you are using to find out how it reacts to cancellations. Typically, there is a special exception type thrown in case of cancellations, e.g. `clients::http::CancelException`.

### How the outside world sees the task's cancellation

The general theme is that a task's completion upon cancellation is still a completion. The task's function will ultimately return or throw something, and that is what the parent task will receive in `engine::TaskWithResult::Get` or `engine::TaskBase::Wait`.

If the cancellation is due to the parent task being cancelled, then its `engine::TaskWithResult::Get` or `engine::TaskBase::Wait` will throw an `engine::WaitInterruptedException`, leaving the child task running (for now), so the parent task will likely not have a chance to observe the child task's completion status. Usually the stack unwinding in the parent task then destroys the `engine::Task` handle, which causes it to be cancelled and awaited.

@snippet core/src/engine/task/cancel_test.cpp parent cancelled

If the child task got cancelled without the parent being cancelled, then:

  * `engine::TaskWithResult::Get` will return or throw whatever the child task has returned or thrown, which is practically meaningless (because why else would someone cancel a task?);
  * `engine::TaskBase::Wait` will return upon completion;
  * `engine::TaskBase::IsFinished` will return `true` upon completion;
  * `engine::TaskBase::GetStatus` will return `engine::TaskBase::Status::kCancelled` upon completion.

There is one extra quirk: if the task is cancelled before being started, then only the functor's destructor will be run by default. See details in `utils::Async`. In this case `engine::TaskWithResult::Get` will throw `engine::TaskCancelledException`.

Tasks launched via `utils::CriticalAsync` are always started, even if cancelled before entering the function. The cancellation will take effect immediately after the function starts:

@snippet core/src/engine/task/cancel_test.cpp critical cancel

### Lifetime of a cancelled task

Note that the destructor of `engine::Task` cancels and waits for task to finish if the task has not finished yet. Use `concurrent::BackgroundTaskStorage` to continue task execution out of scope.

The invariant that the task only runs within the lifetime of the `engine::Task` handle or `concurrent::BackgroundTaskStorage` is the backbone of structured concurrency in userver, see `utils::Async` and `concurrent::BackgroundTaskStorage` for details.

### Utilities that interact with cancellations

The user is provided with several mechanisms to control the behavior of the application in case of cancellation:

  * `engine::current_task::CancellationPoint()` -- if the task is canceled, calling this function throws an exception that is not caught during normal exception handling (not inherited from `std::exception`). This will result in stack unwinding with normal destructor calls for all local objects. The parent task will receive `engine::TaskCancelledException` from `engine::TaskWithResult::Get`.
  **⚠️🐙❗ Catching this exception results in UB, your code should not have `catch (...)` without `throw;` in the handler body**!
  * `engine::current_task::ShouldCancel()` and `engine::current_task::IsCancelRequested()` -- predicates that return `true` if the task is canceled:
    * by default, use `engine::current_task::ShouldCancel()`. It reports that a cancellation was requested for the task and the cancellation was not blocked (see below);
    * `engine::current_task::IsCancelRequested()` notifies that the task was canceled even if cancellation was blocked; effectively ignoring caller's requests to complete the task regardless of cancellation.
  * `engine::TaskCancellationBlocker` -- scope guard, preventing cancellation in the current task. As long as it is alive all the blocking calls are not interrupted, `engine::current_task::CancellationPoint` throws no exceptions, `engine::current_task::ShouldCancel` returns `false`.
    **⚠️🐙❗ Disabling cancellation does not affect the return value of `engine::current_task::IsCancelRequested()`.**


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/intro_io_bound_coro.md | @ref scripts/docs/en/userver/framework_comparison.md ⇨
@htmlonly </div> @endhtmlonly

@example core/src/engine/mutex_benchmark.cpp
