## The Basics

## Restrictions

Usage of `catch (...)` without `throw;` **should be avoided** as the framework
may use exceptions not derived from `std::exception` to manage some resources.
Usage of `catch` with explicit exception type specification (like
`std::exception` or `std::runtime_error`) is fine without `throw;`.

The use of synchronization primitives or IO operations of the C++ standard
library and libc in the context of coroutine **should be avoided**.

🐙 **userver** uses its own coroutine scheduler, which is unknown to the C++
standard library, as well as to the libc. The standard library for
synchronization often uses mutexes, other synchronization primitives and event
waiting mechanisms that block the current thread. When using userver, this
results in the current thread not being able to be used to execute other
coroutines. As a result, the number of threads executing coroutines decreases.
This can lead to a huge performance drops and increased latencies.

For the reasons described above, the use of synchronization primitives or IO
operations of the C++ standard library and libc in the context of coroutine
should be avoided. The same goes for all functions and classes that use such
synchronization primitives.

**⚠️🐙❗ Instead of the standard primitives, you need to use the primitives from the userver:**

| Standard primitive                | Replacement from userver                        |
|-----------------------------------|-------------------------------------------------|
| `std::this_thread::sleep_for()`   | `engine::SleepFor()`                            |
| `std::this_thread::sleep_until()` | `engine::SleepUntil()`                          |
| `std::mutex`                      | `engine::Mutex`                                 |
| `std::shared_mutex`               | `engine::SharedMutex`                           |
| `std::condition_variable`         | `engine::ConditionVariable`                     |
| `std::future<T>`                  | `engine::TaskWithResult<T>` or `engine::Future` |
| `std::async()`                    | `utils::Async()`                                |
| `std::thread`                     | `utils::Async()`                                |
| `std::counting_semaphore`         | `engine::Semaphore`                             |
| network sockets                   | `engine::io::Socket`                            |
| `std::filesystem::`                | `::fs::*` (but not `::fs::blocking::*`!)        |
| `std::cout`                       | `LOG_INFO()`                                    |
| `std::cerr`                       | `LOG_WARNING()` and `LOG_ERROR()`               |

An overview of the main synchronization mechanisms is available [on a separate page](scripts/docs/en/userver/synchronization.md).

______
⚠️🐙❗ If you want to run code that uses standard synchronization primitives
(for example, code from a third-party library), then this code should be run in
a separate `engine::TaskProcessor` to avoid starvation of main task processors.
See @ref md_en_userver_task_processors_guide for more info.
______


## Tasks
The asynchronous **task** (`engine::Task`, `engine::TaskWithResult`) can return
a result (possibly in form of an exception) or return nothing. In any case, the
task has the semantics of future, i.e. you can wait for it and get the result
from it.

To create a task call the `utils::Async` function. It accepts the name of a
task, the user-defined function to execute, and the arguments of the
user-defined function:

```cpp
auto task = utils::Async("my_job", &func, arg1, arg2);
// do something ...
auto result = task.Get();
```


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
Task can be notified that it needs to discard its progress and finish early. Cancelling a task restricts the execution of blocking operations in that task.

To cancel a task, just call the `engine::Task::RequestCancel()` or `engine::Task::SyncCancel()` method. It cancels only a single task and does not affect the subtasks that were created by the canceled task. The framework typically reports a wait interrupt either by using a return code (for example, `CvStatus` for `engine::ConditionVariable::Wait`), or by throwing a module-specific exception (`WaitInterruptedException` for `engine::Task::Wait`).

In addition to explicitly calling `engine::Task::RequestCancel()`, cancellation can occur:
  * due to application shutdown;
  * due to the end of the `engine::Task` lifetime;
  * for other reasons (lack of resources, task hanging, etc.).

The user is provided with several mechanisms to control the behavior of the application in case of cancellation:
  * `engine::current_task::CancellationPoint()` -- if the task is canceled, calling this function throws an exception that is not caught during normal exception handling (not inherited from `std::exception`). This will result in stack unwinding with normal destructor calls for all local objects.
  **⚠️🐙❗ Catching this exception results in UB, your code should not have `catch (...)` without `throw;` in the handler body**!
  * `engine::current_task::ShouldCancel()` and `engine::current_task::IsCancelRequested()` -- predicates that return `true` if the task is canceled:
    * by default, use `engine::current_task::ShouldCancel()`. It reports that a cancellation was requested for the task and the cancellation was not blocked (see below);
    * `engine::current_task::IsCancelRequested()` notifies that the task was canceled even if cancellation was blocked; effectively ignoring caller's requests to complete the task regardless of cancellation.
  * `engine::TaskCancellationBlocker` -- scope guard, preventing cancellation in the current task. As long as it is alive all the blocking calls are not interrupted, `engine::current_task::CancellationPoint` throws no exceptions, `engine::current_task::ShouldCancel` returns `false`.
    **⚠️🐙❗ Disabling cancellation does not affect the return value of `engine::current_task::IsCancelRequested()`.**

Calling `engine::TaskWithResult::Get()` on a canceled task would wait for task to finish and a `engine::TaskCancelledException` exception would be thrown afterwards.
For non-canceled tasks the `engine::TaskWithResult::Get()` returns the result of the task.

Note that the destructor of `engine::Task` cancels and waits for task to finish if the task has not finished yet. Use `concurrent::BackgroundTaskStorage` or `engine::Task::Detach()` to continue task execution out of scope.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_intro_io_bound_coro | @ref md_en_userver_framework_comparison ⇨
@htmlonly </div> @endhtmlonly

@example core/src/engine/mutex_benchmark.cpp
