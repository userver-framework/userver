## I/O-bound Applications and Coroutines

## Microservices and other I/O-bound applications

For [microservices](https://en.wikipedia.org/wiki/Microservices) waiting for
I/O is typical: often the response of a microservice is formed from several
responses from other microservices and databases.

The problem of efficient waits for I/O in the classical approach is solved by
callbacks: a function (called a callback) is passed to the method that performs
I/O, the callback is called by the method when the wait is complete. If you need
to perform several I/O operations sequentially, the callback from the first I/O
method calls the method for the next I/O and passes the next callback to it.
As a result, you get code that is unpleasant to write and difficult to maintain
due to the many nested functions and non-obvious control flow.

The userver framework with stackful coroutines comes to the rescue.

For the user of the framework, **the code becomes simple and linear,
but everything works efficiently**:

```cpp
Response View::Handle(Request&& request, const Dependencies& dependencies) {
  auto cluster = dependencies.pg->GetCluster();                             // 🚀
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

For the simplicity of the example, all lines where the coroutine can be paused
are marked as `// 🚀`. In other frameworks or programming languages it is often
necessary to explicitly mark the context switch of the coroutine. In those
languages in each line with the comment `// 🚀`, you would have to write a
keyword like `await`. In userver you do not need to do this, switching occurs
automatically and you do not need to think about the implementation details of
various methods of the framework.

Unlike in the Python, **in userver multiple coroutines can be executed
simultaneously on different processor cores**. For example the `View::Handle`
may be called in parallel for different requests.

Now compare the above userver code with the classic callback approach:
```cpp
void View::Handle(Request&& request, const Dependencies& dependencies, Response response) {
  dependencies.pg->GetCluster(
    [request = std::move(request), response](auto cluster)
  {
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
  });
}
```

The classical approach is almost twice as long, and it is difficult to read and
maintain because of the deep nesting of the lambda functions.
Moreover, the time-consuming error codes handling is completely omitted, while
in the first example all the errors are automatically reported through the
exception mechanism.



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
|-------------------|--------------|---------|
| 1                 | 22 ns        | 19 ns   |
| 2                 | 205 ns       | 154 ns  |
| 4                 | 403 ns       | 669 ns  |



----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
| @ref md_en_userver_intro ⇨
@htmlonly </div> @endhtmlonly
