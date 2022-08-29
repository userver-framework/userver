## Synchronization Primitives

This page describes the synchronization mechanisms that are available in userver, their advantages and weaknesses. 
It is assumed that the developer is aware of concurrent programming and concepts like "race", "critical section", "mutex".

## Constraint
⚠️🐙❗ Use of the C++ standard library and libc synchronization primitives in coroutines **is forbidden**.

## Synchronization mechanisms and primitives

This section describes the major available synchronization mechanisms with use cases. All the primitives are listed at the @ref userver_concurrency API Group.

@note There is no "faster" synchronization mechanism. Different primitives are suitable for different situations. 
For some load profile primitive A could be 1000 times faster than primitive B. For another - exactly the opposite. 
Before choosing a primitive, determinate the load profile 
(how often does reading and writing occur, 
how many concurrent writers/readers are there etc). 
For a practical assessment of the effectiveness of a particular tool for your case, use 
[google benchmark](https://github.com/google/benchmark) and stress testing.


### engine::TaskWithResult

For parallel independent calculations the simplest, and most reliable way to transmit data is through the result of the engine::TaskWithResult execution.

@snippet engine/task/task_with_result_test.cpp  Sample TaskWithResult usage

A less convenient and more complicated way to solve the same problem is to create a data structure shared between tasks, where the tasks themselves will record the result. This requires protecting the data through atomic variables or engine::Mutex, as well as passing this data structure to the subtasks. In this case, engine::Future may be useful (see below).

Note that when programming tasks, you need to take into account the lifetime of objects. If you pass a closure to a task with a reference to a variable, then you must ensure that the lifetime of the task is strictly less than the lifetime of the variable. This must also be guaranteed for the case of throwing an exception from any function used. If this cannot be guaranteed, then either pass the data to the closure via shared_ptr, or pass it over the copy.


### engine::Future

Sometimes calculations could not decomposed easily and a single engine::Task should return many results. At the same time, it is not efficient to collect them into one structure and return them at one (the results are used as inputs for several tasks, etc.).
For such cases, you can use the engine::Promise and engine::Future. They provide a synchronized channel for transmitting the value between tasks.
The interface and contracts of these classes are as close as possible to similar types from the standard library. The main differences are related to the support for the cancellation mechanism.

@snippet engine/future_test.cpp  Sample engine::Future usage

In this case, the main mechanism for transmitting data remains the return of values from TaskWithResult.


### engine::WaitAny and friends

The most effective way to wait for one of the asynchronous operations is to use
engine::WaitAny, engine::WaitAnyFor or engine::WaitAnyUntil:

@snippet src/engine/wait_any_test.cpp sample waitany

It works with different types of tasks and futures. For example could be used
to get the ready HTTP requests ASAP:

@snippet src/clients/http/client_wait_test.cpp HTTP Client - waitany

See also engine::WaitAllChecked and engine::GetAll for a way to wait for all
of the asynchronous operations, rethrowing exceptions immediately.


### concurrent::MpscQueue

For long-living tasks it is convenient to use message queues.
In `concurrent::MpscQueue`, writers (one or more) can write data to the queue, and on the other hand, a reader can read what is written. The order of objects written by different writers is not defined.

@snippet concurrent/mpsc_queue_test.cpp  Sample concurrent::MpscQueue usage

If the queue is supposed to pass data types `T` with a non-trivial destructor, then you need to use the queue `concurrent::MpscQueue<std::unique_ptr<T>>`. If the queue with unread data is destroyed, all unprocessed items will be released correctly.

Use this class by default. However, if you really need higher performance use NonFifo queues:

* `concurrent::NonFifoMpmcQueue`
* `concurrent::NonFifoMpscQueue`
* `concurrent::NonFifoSpmcQueue`
* `concurrent::NonFifoSpscQueue`

NonFifo queues do not guarantee FIFO order of the elements of the queue and thereby have higher performance.

### std::atomic

If you need to access small trivial types (`int`, `long`, `std::size_t`, `bool`) in shared memory from different tasks, then atomic variables may help. Beware, for complex types compiler generates code with implicit use of synchronization primitives forbidden in userver. If you are using `std::atomic` with a non-trivial or type parameters with big size, then be sure to write a test to check that accessing this variable does not impose a mutex.

It is not recommended to use non-default memory_orders (for example, acquire/release), because their use is fraught with great difficulties. In such code, it is very easy to get bug that will be extremely difficult to detect. Therefore, it is better to use a simpler and more reliable default, the std::memory_order_seq_cst.

### engine::Mutex

A classic mutex. It allows you to work with standard `std::unique_lock` and `std::lock_guard`.

@snippet engine/mutex_test.cpp  Sample engine::Mutex usage


Prefer using `concurrent::Variable` instead of an `engine::Mutex`.


### engine::SharedMutex

A mutex that has readers and writers. It allows you to work with standard `std::unique_lock`,`std::lock_guard` and `std::shared_lock`.

@snippet engine/shared_mutex_test.cpp  Sample engine::SharedMutex usage

engine::SharedMutex is a much more complex and heavier synchronization primitive than a regular engine::Mutex. The fact of the existence of readers and writers does not mean that engine::SharedMutex will be faster than an  engine::Mutex. For example, if the critical section is small (2-3 integer variables), then the SharedMutex overhead can outweigh all the gain from concurrent reads. Therefore, you should not mindlessly use SharedMutex without benchmarks. Also, the "frequent reads, rare writes" scenario in most cases is solved much more efficiently through RCU - `rcu::Variable`.

Read locking of the SharedMutex is slower than reading an `RCU`. However, SharedMutex does not require copying data on modification, unlike `RCU`. Therefore, in the case of expensive data copies that are protected by a critical section, it makes sense to use SharedMutex instead of `RCU`. If the cost of copying is low, then it is usually more profitable to use `RCU`.

To work with a mutex, we recommend using `concurrent::Variable`. This reduces the risk of taking a mutex in the wrong mode, the wrong mutex, and so on.


### rcu::Variable

A synchronization primitive with readers and writers that allows readers to work with the old version of the data while the writer fills in the new version of the data. Multiple versions of the protected data can exist at any given time. The old version is deleted when the RCU realizes that no one else is working with it. This can happen when writing a new version is finished if there are no active readers. If at least one reader holds an old version of the data, it will not be deleted.


RCU should be the "default" synchronization primitive for the case of frequent readers and rare writers. Very poorly suited for frequent updates, because a copy of the data is created on update.

@snippet rcu/rcu_test.cpp  Sample rcu::Variable usage

Comparison with SharedMutex is described in the `engine::SharedMutex` section of this page.


### rcu::RcuMap

`rcu::Variable` based map. This primitive is used when you need a concurrent dictionary. Well suited for the case of rarely added keys. Poorly suited to the case of a frequently changing set of keys.

Note that RcuMap does not protect the value of the dictionary, it only protects the dictionary itself. If the values are non-atomic types, then they must be protected separately (for example, using `concurrent::Variable`).

@snippet rcu/rcu_map_test.cpp  Sample rcu::RcuMap usage

### concurrent::Variable

A proxy class that combines user data and a synchronization primitive that protects that data. Its use can greatly reduce the number of bugs associated with incorrect use of the critical section - taking the wrong mutex, forgetting to take the mutex, taking SharedMutex in the wrong mode, etc.

@snippet concurrent/variable_test.cpp  Sample concurrent::Variable usage

### engine::Semaphore

The semaphore is used to limit the number of users that run inside a critical section. For example, a semaphore can be used to limit the number of simultaneous concurrent attempts to connect to a resource.

@snippet engine/semaphore_test.cpp  Sample engine::Semaphore usage

You don't need to use a semaphore if you need to limit the number of threads that perform CPU-heavy operations. For these purposes, create a separate TaskProcessor and perform other operations on it, it is be cheaper in terms of synchronization.

If you need a counter, but do not need to wait for the counter to change, then you need to use `std::atomic` instead of a semaphore.

### engine::SingleUseEvent

A single-producer, single-consumer event without task cancellation support. Must not be awaited or signaled multiple times in the same waiting session.

For multiple producers and cancellation support, use `engine::SingleConsumerEvent` instead.

@snippet engine/single_use_event_test.cpp  Sample engine::SingleUseEvent usage

### utils::SwappingSmart

**Don't use** `utils::SwappingSmart`, use `rcu::Variable` instead. There is a UB in the SwappingSmart behavior.

`utils::SwappingSmart` protects readers from rare writers, but in the case of very frequent writers, the reader has no guarantee of getting a valid `std::shared_ptr` (which occasionally fired in production). Also `rcu::Variable` faster than `utils::SwappingSmart` in most cases.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref userver_components | @ref md_en_userver_formats ⇨
@htmlonly </div> @endhtmlonly
