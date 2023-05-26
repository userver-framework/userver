# Frequently Asked Questions

### Why or When to use userver

🐙 **userver** perfectly suits IO-bound networking applications that require
* high efficiency
* and/or rich functionality within C++ language
* and/or widely tested reliable code-base


### The framework does not build

If you are trying to build the framework on Windows OS, you should use WSL
as the platform native API
[is not supported at the moment](https://github.com/userver-framework/userver/issues/228).

Try disabling modules that you do not use, see @ref md_en_userver_tutorial_build
for a list of supported CMake options.

If you have problems with PostgreSQL build, see @ref POSTGRES_LIBS "PostgreSQL versions".


### Service Terminated/Aborted/SIGSEGV. What to do?

#### Hint: Take a look at the service logs

Usually a fix-it hint could be found in logs:
```
tskv	timestamp=2023-08-13T15:30:52.507493	level=ERROR	module=BackgroundTaskStorageCore::CancelAndWait() ( userver/core/src/concurrent/background_task_storage.cpp:139 )	text=CancelAndWait should be called no more than once	stacktrace=<some stacktrace>
```

Otherwise, there could be enough information to reproduce the problem.


#### Hint: Take a look at the core dump or stacktrace.

* `std::terminate` in core dump usually means that the an exception was thrown from
  a `noexcept` function. See the trace for the place where that happened and add
  `try`+`catch` block in your sources, to catch and print the exception that
  is thrown.
* Take a closer look at the utils::Async and engine::AsyncNoSpan usage in
  your code. Captured by reference variables in lambdas should outlive the
  returned task.

  Wrong:
  ```cpp
  engine::Task task1;
  engine::Task task2;

  std::string data = "I store some heap allocated data";
  task1 = utils::Async("task1", [&data](){ function1(data); });
  task2 = utils::Async("task2", [&data](){ function2(data); });
  
  task2.Get(); // oops! The exception from Get() would call the destructor
               // of `data` while `task1` still uses it.
  ```
  Fixed:
  ```cpp
  std::string data = "I store some heap allocated data";
  auto task1 = utils::Async("task1", [&data](){ function1(data); });
  auto task2 = utils::Async("task2", [&data](){ function2(data); });
  
  task2.Get(); // `task1` and `task2` cancelled, waited and destroyed
               // before destruction of `data`.
  ```


### Service is waiting for something

#### Hint: Take a look at the service logs

In logs could be enough information to reproduce the problem.


#### Hint: Localize the thread that uses the most CPU

Command like `top -b -H -n 3` would output the top CPU consuming threads. All
the userver threads have reasonable names, that allow you to distinguish between
different task processors and supplementary threads for logging/IO-processing/...

This could be handy in detecting infinite loops or CPU intensive computations.


#### Hint: Grab a stacktrace from a running service

Command like `gdb -batch -ex 'thread apply all bt full' -p PID_OF_THE_SERVICE`
should output a detailed information on each thread. Search for


#### Hint: Take a look at the metrics

If some metric starts to grow when the slowdown/wait starts - it's a good
hint on a problem.

For example, if `engine.coro-pool.coroutines.active` metric has a burst-like
grow - then the somewhere a lot of tasks was produced and the task processor
tries hard to deal with those.

If `major_pagefaults` metric grows and CPU usage of
@ref md_en_userver_task_processors_guide "main task processors" is small,
then a blocking filesystem operation is executed in the main task processor.
Investigate the code of your service and move those operations to
@ref md_en_userver_task_processors_guide "fs-task-processor".


If there's no suspicious metrics grows and the CPU usage of main task
processors is small - then probably there is a blocking system call in the code
of your service. Locate it and replace it with a
@ref md_en_userver_intro "proper replacement" from userver.


### PostgreSQL: Statement XXXX network timeout error

Database server failed to answer in time. If in logs you see a timeout that
is less than your network timeout – it's not an error. It is the time
left after a connection was retrieved from connection pool:
```
acquire connection + execute query <= network timeout
```

See PostgreSQL related
@ref md_en_schemas_dynamic_configs for more info.


### PostgreSQL: Statement XXXX was canceled

Statement was canceled by the `statement timeout`. See PostgreSQL related
@ref md_en_schemas_dynamic_configs for more info.


### PostgreSQL: Something is slow

Take a look at the PostgreSQL related metrics.

Errors related to the `*.pool` mean that all the connections in pool are busy
and a spare connection fails to appear in specified timeout. Values close to
the `network timeout` in the `*.acquire-connection` metrics mean the same thing.

Big values in `*.return-to-pool` mean that it takes a lot of time to clean up
the connection to make it `idle` again. Most probably, there are problems with
network.

Big values in the `*.busy` metrics mean that the query is slow to execute on
the database server or that there are problems with network.

See PostgreSQL metrics descriptions at @ref md_en_userver_service_monitor.


### PostgreSQL: Timestamp in PostgreSQL and locally differ for multiple hours/minutes

When choosing the data type for storing date/time, you should always give
preference to `timestamp WITH time zone` (aka `timestampz`), unless there is a
very good reason to use `timestamp WITHOUT time zone` (aka `timestamp`).
There is no saving on size, both types on disk (and when transmitted in binary
form) take 8 bytes.

The main reason why you should not use `timestamp WITHOUT time zone` – absolute
unpredictability of values of different clients, which may have different time
zones on their machines.

```sql
db=# show TimeZone;
   TimeZone
---------------
 Europe/Moscow
(1 row)

db=# create temporary table tztest(no_tz timestamp, tz timestamptz);
CREATE TABLE

db=# insert into tztest values(current_timestamp, current_timestamp);
INSERT 0 1

db=# select current_timestamp, * from tztest;
       current_timestamp       |           no_tz            |              tz
-------------------------------+----------------------------+-------------------------------
 2019-04-24 14:11:24.997834+03 | 2019-04-24 14:11:21.554988 | 2019-04-24 14:11:21.554988+03
(1 row)

db=# set TimeZone to 'Asia/Vladivostok';
SET

db=# select current_timestamp, * from tztest;
       current_timestamp       |           no_tz            |              tz
-------------------------------+----------------------------+-------------------------------
 2019-04-24 21:11:32.355442+10 | 2019-04-24 14:11:21.554988 | 2019-04-24 21:11:21.554988+10
(1 row)
```

The only reason when the `timestamp WITHOUT time zone` should be used
is when this column is used in
[partitioning tables in where clause](https://www.postgresql.org/docs/current/ddl-partitioning.html#DDL-PARTITIONING-CONSTRAINT-EXCLUSION).
At the same time, it is necessary to ensure that the value is written in `UTC`
(for example, `current_timestamp at time zone 'UTC'`).

The current implementation in the driver maps std::chrono::system_block::time_point
to `timestamp WITHOUT time zone`, so when used in queries, do not forget to add
`AT TIME ZONE 'UTC'` to fields that have timezone. For the convenience of using
the `timestamp WITH timezone` type in C++ a strong typedef is declared for
std::chrono::system_block::time_point with the name
storages::postgres::TimePointTz.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_roadmap_and_changelog | @ref md_en_userver_tutorial_hello_service ⇨
@htmlonly </div> @endhtmlonly
