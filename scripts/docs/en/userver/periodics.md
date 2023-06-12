# Periodics and DistLocks

Basic information on functions and classes that execute some code triggered by
some non-IO event.

### PeriodicTask

utils::PeriodicTask, as the name implies, regularly calls user code. Keep in
mind that the task is called on every machine in the cluster.

Well suited for:
* background actions within the component
* cheap idempotent database cleanup, provided that repeated cleaning is very
  cheap for the database; if this is not the case, then use
  dist_lock::DistLockedTask.

Poorly suited for:
* Regularly receiving data from another service for caching purposes.
  To do this, it is better to use specially designed
  @ref md_en_userver_caches "caches".
* Background DB accesses for periodic changes. Use instead dist_lock::DistLockedTask.

The update period can be changed without stopping the periodic task using
utils::PeriodicTask::SetSettings().

Sample:
```cpp
Component::Component(...)
{
  std::chrono::seconds interval{40}; // every 40 seconds
  PeriodicTask::Settings settings{interval};
  syncer_.Start("task_name", settings, [this]{ DoSomeWork(); });

  // Register itself in testsuite
  periodic_task_.RegisterInTestsuite(
      context.FindComponent<::components::TestsuiteSupport>()
          .GetPeriodicTaskControl());
}
```

See @ref TESTSUITE_TASKS "Testsuite tasks" for information about functional
testing of such tasks.


### DistLock

A family of tools for background task execution on only one machine in a
cluster. When dist lock starts, the lock worker tries to take a lock in the
loop. If succeeded, a task is launched that executes the user code.
In the background, dist lock tries to extend the lock. In case of loss of the
lock, the user task is canceled.

The implementation has some protection against brain split (more than one
instance believes that they hold the lock, and do work that requires a lock).
Watchdog periodically checks whether we were able to extend the lock, and in
case of an imminent loss of the lock, cancels the user task. The user task
itself is responsible for stopping the work in case of cancellation
(via engine::current_task::ShouldCancel()). If the task continues to work
despite the cancellation, then a brain split will happen and the task will
start to execute on multiple instances at the same time.

Lock implementation options:
* via Postgres using storages::postgres::DistLockComponentBase.
* via Mongo using storages::mongo::DistLockComponentBase.
* through some special service using the dist_lock::DistLockedWorker component.

It is well suited for:
* heavy operation that is performed for a long time, while you do not want to
  execute it on two or more cluster machines at once
* operations that cannot be executed concurrently on several machines
  (for example, a distributed transaction in several databases).

Poorly suited if:
* strict guarantees for parallel startup are not required
* brain split is categorically unacceptable
* an explicit synchronization point in the form of a lock storage is not
  suitable due to reliability / performance / adding a bottleneck.
* Execution of a non periodic task that should hold a distributed lock.
  Use dist_lock::DistLockedTask instead.

Sample:
```cpp
class Worker: public ::storages::postgres::DistLockComponentBase { ... }

Worker::Worker(...)
{
  auto& testsuite_tasks = testsuite::GetTestsuiteTasks(context);
  if (testsuite_tasks.IsEnabled()) {
    testsuite_tasks.RegisterTask(kName, [this] { DoWork(); });
  } else {
    AutostartDistLock();
  }
}

void Worker::DoWork() {
  // do a periodic work step
}
```

See @ref TESTSUITE_TASKS "Testsuite tasks" for information bout functional
testing of such tasks.


#### Recommendations on DoWork implementation

If work in `DoWork` takes a long time, then it is necessary to check for task
cancellation at least once in `lock-ttl` via
engine::current_task::ShouldCancel(). Synchronization primitives
engine::InterruptibleSleepFor, engine::SingleConsumerEvent, engine::Future,
userver queues, requests to clients - support cancellations, that interrupt
waiting when the task is canceled. After such a wait, you need to check whether
the task has been canceled - synchronization primitives throw an exception or
return `false` from a wait operation, depending on the primitive.
After engine::InterruptibleSleepFor you should call
engine::current_task::ShouldCancel() manually.

In 'DoWork` you can perform one step of the operation, or you can write a loop
`while (!ShouldCancel())`. You can choose an approach based on this principle:

@note If it is important to perform a distlock on the same host, then write a loop
      `DoWork`, otherwise perform a single iteration.

More detailed:
* If switching the host is cheap, then it is better not to write a loop in
  `DoWork`, then the work has a chance to be distributed among the hosts
  (although there is no guarantee). If in doubt, it is better not to write a
  loop in `DoWork'.
* In some cases, the distlock performs one big task with interruptions via
  engine::InterruptibleSleepFor to not overload the CPU and/or the database.
  Then you need to write a loop inside DoWork. Or if the host needs an
  expensive setup before starting the work on a distlock task, then you can
  also write a loop.

#### Guarantees provided

The custom coroutine that runs DoWork is always the same. If the cancellation
is not processed, then another DoWork-coroutine is not launched.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_task_processors_guide | @ref md_en_userver_testing ⇨
@htmlonly </div> @endhtmlonly
