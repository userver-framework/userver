## Basic information



## Introduction

This page describes the core of the asynchronous framework (userver). It allows you to write code in C++ using coroutines.

Repository: TODO: https://github.yandex-team.ru/taxi/userver

Support chat: TODO: https://nda.ya.ru/t/c7vovbuM3VyoiK


## Microservices and userver

For [microservices](https://en.wikipedia.org/wiki/Microservices) waiting for I/O is typical: often the response of a microservice is formed from several responses from other microservices and databases.

The problem of effectively waiting for I/O in the classical approach is solved by callbacks: a function (called a callback) is passed to the method that performs I/O, the callback is called by the method when the wait is complete. If you need to perform several I/O operations sequentially, the callback from the first I/O method calls the method for the next I/O and passes the next callback to it. As a result, you get code that is unpleasant to write and difficult to maintain due to the many nested functions and non-obvious control flow.

The userver framework with stackful coroutines comes to the rescue. For the user of the framework, **the code becomes simple and linear, but everything works efficiently**:

```
cpp
Response View::Handle(Request&& request, const Dependencies& dependencies) {
  auto cluster = dependencies.pg->GetCluster();
  auto trx = cluster->Begin(storages::postgres::ClusterHostType::kMaster);  // 🚀

  const char* statement = "SELECT ok, baz FROM some WHERE id = $1 LIMIT 1";
  auto row = psql::Execute(trx, statement, request.id)[0];                  // 🚀
  if (!row["ok"].As<bool>()) {
    LOG_DEBUG() << request.id << " is not OK of "
                << GetSomeInfoFromDb();                                     // 🚀
    return Response400();
  }

  psql::Execute(trx, queries::kUpdateRules, request.foo, request.bar);      // 🚀
  trx.Commit();                                                             // 🚀

  return Response200{row["baz"].As<std::string>()};
}
```

For the simplicity of the example, all lines where the coroutine can be paused are marked as `// 🚀`. In other frameworks or programming languages it is often necessary to explicitly mark the context switch of the coroutine. In those languages in each line with the comment `// 🚀`, you would have to write a keyword like `await`. In userver you do not need to do this, switching occurs automatically and you do not need to think about the implementation details of various methods of the framework.

Unlike in the Python, **in userver multiple coroutines can be executed simultaneously on different processor cores**. For example the `View::Handle` may be called in parallel for different requests.

Now compare the above userver code with the classic callback approach:
```
cpp
void View::Handle(Request&& request, const Dependencies& dependencies, Response response) {
  auto cluster = dependencies.pg->GetCluster();

  cluster->Begin(storages::postgres::ClusterHostType::kMaster,
    [request = std::move(request), response](auto& trx)
  {
    const char* statement = "SELECT ok, baz FROM some WHERE id = $1 LIMIT 1";
    psql::Execute(trx, statement, request.id,
      [request = std::move(request), response, trx = std::move(trx)](auto& res)
    {
      auto row = res[0];
      if (!row["ok"].As<bool>()) {
        if (LogDebug()) {
            GetSomeInfoFromDb([id = request.id](auto info) {
                LOG_DEBUG() << id << " is not OK of " << info;
            });
        }    
        *response = Response400{};
      }

      psql::Execute(trx, queries::kUpdateRules, request.foo, request.bar, 
        [row = std::move(row), trx = std::move(trx), response]()
      {
        trx.Commit([row = std::move(row), response]() {
          *response = Response200{row["baz"].As<std::string>()};
        });
      });
    });
  });
}
```

The classical approach is almost twice as long, and it is difficult to read and maintain because of the deep nesting of the lambda functions.
Moreover, the time-consuming error codes handling is completely omitted, while in the first example all the errors are automatically reported through the exception mechanism.



## Coroutines
In simple terms, coroutines are lightweight threads that are managed by the application itself, not by the operating system.

Coroutines provide an additional benefit in high-load applications:
* The OS does not need to know about coroutines, and therefore does not need to use complex and heavy logic to manage them and spend additional system resources.
* The application knows how and when it is best to pause or start the coroutine, thus the engine avoids some of the heavy OS context switches.
* Switching between coroutines is an easy operation that usually does not require system calls or other heavy operations.

However, there is a disadvantage of a cooperative multitasking:
* Coroutine scheduler is not preemptive. Poorly written coroutine payloads could occupy the system threads and make the other coroutines starve,
but in most cases there are means to make them cooperate better with minimal code changes.

The user of the framework does not work directly with the coroutines, but rather works with tasks that are executed on the coroutines. It is very expensive to create and destroy coroutines for each asynchronous operation, so coroutines are reused between tasks. When a new task is starting, a free coroutine is selected to host it. After the task is finished, the coroutine is released, and can be used by another task.


## Performance
The main purpose of userver is to be an effective solution for IO-bound applications. Coroutines help to achieve that purpose.

It takes less than 1us to invoke and wait for a noop task `engine::Async([] () {}).Wait()`. In this case, the task execution time will be automatically measured inside the `engine::Async()`, the task will be linked to the parent task to simplify [tracing, all information will be recorded in logs](scripts/docs/en/userver/logging.md).

For comparison, `std::thread ([] () {}).join()` takes ~17us and does not provide or log any information.

The userver synchronization primitives are comparable in performance to standard primitives, and we are constantly working to improve them. For example @ref core/src/engine/mutex_benchmark.cpp "concurrent `lock()` and `unlock()` of the same mutex from different threads on a 2 core system with Hyper-threading" produce the following timings:

| Competing threads | `std::mutex` | `Mutex` |
|-------------|-------------|-------------|
| 1 | 22 ns | 19 ns |
| 2 | 205 ns | 154 ns |
| 4 | 403 ns | 669 ns |


## Restrictions

Usage of `catch (...)` without `throw;` **should be avoided** as the framework may use exceptions not derived from `std::exception` to manage some resources. Usage of `catch` with explicit exception type specification (like `std::exception` or `std::runtime_error`) is fine without `throw;`.

The use of synchronization primitives or IO operations of the C++ standard library and libc in the context of coroutine **should be avoided**.

Userver uses its own coroutine scheduler, which is unknown to the C++ standard library, as well as to the libc. The standard library for synchronization often uses mutexes, other synchronization primitives and event waiting mechanisms that block the current thread. When using userver, this results in the current thread not being able to be used to execute other coroutines. As a result, the number of threads executing coroutines decreases. This can lead to a huge performance drops and increased latencies.

For the reasons described above, the use of synchronization primitives or IO operations of the C++ standard library and libc in the context of coroutine should be avoided. The same goes for all functions and classes that use such synchronization primitives.

**⚠️🐙❗ Instead of the standard primitives, you need to use the primitives from the userver:**

| Standard primitive | Replacement from userver |
|-------------|-------------|
| `std::this_thread::sleep_for()` | `engine::SleepFor()` |
| `std::this_thread::sleep_until()` | `engine::SleepUntil()` |
| `std::mutex` | `engine::Mutex` |
| `std::condition_variable` | `engine::ConditionVariable` |
| `std::future<void>` and `std::future<T>` | `engine::Task` and `engine::TaskWithResult<T>` or `engine::Future` |
| `std::async()` | `utils::Async()` |
| `std::thread` | `utils::Async()` |
| network sockets | `engine::io::Socket` |
| `std::filesystem:` | `::fs::*` (but not `::fs::blocking::*`!) |
| `std::cout` | `LOG_INFO()` |
| `std::cerr` | `LOG_WARNING()` and `LOG_ERROR()` |

An overview of the main synchronization mechanisms is available [on a separate page](scripts/docs/en/userver/synchronization.md).

______
⚠️🐙❗ If you want to run code that uses standard synchronization primitives (for example, code from a third-party library), then this code should be run in a
separate `engine::TaskProcessor` to avoid starvation of main task processors.
______


## Tasks
The asynchronous **task** (`engine::Task`, `engine::TaskWithResult`) can return a result (possibly in form of an exception) or return nothing. In any case, the task has the semantics of future, i.e. you can wait for it and get the result from it.

To create a task call the `utils::Async` function. It accepts the name of a task, the user-defined function to execute, and the arguments of the user-defined function:

```
cpp
auto task = utils::Async("my_job", &func, arg1, arg2);
// do something ...
auto result = task.Get();
```


## Waiting

The code inside the coroutine may want to wait for an external event - a response from the database, a response from the HTTP client, the arrival of a certain time. If a coroutine wants to wait, it tells the engine that it wants to suspend its execution, and another coroutine starts executing on the current thread of the operating system instead. As a result, the thread is not idle, but reused by other users. After an external event occurs, the coroutine will be scheduled and executed.

```
cpp
f();
engine::SleepFor(std::chrono::seconds(60)); // voluntarily giving the current thread to other coroutines
g(); // The thread has returned to us
```

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

Note that the destructor of `engine::Task` cancels and waits for task to finish if the task has not finished yet. Use `utils::BackgroundTasksStorage` or `engine::Task::Deatch()` to continue task execution out of scope.

@example core/src/engine/mutex_benchmark.cpp
