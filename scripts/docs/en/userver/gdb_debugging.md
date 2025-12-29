# Debugging with GDB

In the most cases writing extensive logs and running profilers is sufficient for debugging. But in the most complex
cases it may be useful to do interactive debugging with GDB. Additionally, GDB is the only option for debugging
coredumps. 🐙 userver provides the capability to perform such debugging.

You can use GDB on your service based on userver just like any other binary (debug symbols are included
by default in all build types).

If not debugging a production installation, then the environment (databases and configs) could be set up by the
testsuite via `make start`* commands. After that just detach to a running process via GDB.


## Userver-specific debug features

To use userver-specific debug features, you need to allow execution of debug scripts, linked into your binary.
It can be done by adding the following line to your ~/.gdbinit file
```
add-auto-load-safe-path <path-to-your-binary>
```
Alternatively, if you trust all the files you are debugging:
```
add-auto-load-safe-path /
```
See [GDB manual](https://www.sourceware.org/gdb/current/onlinedocs/gdb.html/Auto_002dloading-safe-path.html) for more details.


### Custom pretty-printers

The simplest extensions for GDB, that userver provides, are pretty-printers for certain data structures. Below is an
example comparing the output for a `formats::json::Value` with and without pretty-printers:

```
(gdb) print value
$1 = {["a"] = {1, 2, 3, 4}, ["c"] = 123, ["d"] = false, ["e"] = "", ["f"] = {["key1"] = "value1", ["key2"] = 987}}
(gdb) disable pretty-printer
181 printers disabled
0 of 181 printers enabled
(gdb) print value
$2 = {holder_ = {static kInvalidVersion = 18446744073709551615, data_ = {__ptr_ = 0x10f27fe04118, __cntrl_ = 0x10f27fe04100}},
  root_ptr_for_path_ = 0x10f27fe04118, value_ptr_ = 0x10f27fe04118, depth_ = 0, lazy_detached_path_ = {parent_value_ptr_ = 0x0,
    parent_depth_ = 0, virtual_path_ = {static __endian_factor = 2, __rep_ = {__s = {{__is_long_ = 0 '\000', __size_ = 0 '\000'},
          __padding_ = {<No data fields>}, __data_ = '\000' <repeats 22 times>}, __l = {{__is_long_ = 0, __cap_ = 0}, __size_ = 0,
          __data_ = 0x0}}, __padding1_933_ = {__padding_ = 0x7fffffffd7c0 ""},
      __alloc_ = {<std::__y1::__non_trivial_if<true, std::__y1::allocator<char> >> = {<No data fields>}, <No data fields>}, __padding2_933_ = {
        __padding_ = 0x7fffffffd7c0 ""}, static npos = 18446744073709551615}}}
```

In addition, the output has a hierarchical structure that is displayed correctly when debugging from the IDE.


### Coroutines exploration

🐙 userver provides GDB command `utask`, which mimics `thread` command and allows you to explore all coroutines
(userver tasks), including running and suspended ones, in a manner similar to threads.


#### Commands

* `utask list`: Lists all tasks with their names (corresponding span names) and statuses. Example:
```
(gdb) help utask list 
usage: utask list [-h] [-s [STATE ...]] [-i ID] [-n NAME] [-b BACKTRACE]

List userver tasks (all or some of them)

options:
  -h, --help            show this help message and exit
  -s [STATE ...], --states [STATE ...]
                        List utasks with specific states only (one of {'invalid', 'new', 'queued', 'running', 'suspended', 'cancelled',
                        'completed'})
  -i ID, --id ID        List utask with specific id only
  -n NAME, --name NAME  List utasks which names match the regex
  -b BACKTRACE, --backtrace BACKTRACE
                        List utasks which backtraces match the regex
(gdb) utask list
Task ID        State     Span name
0x10f27fc40800 suspended task_3
0x10f27fc3f000 suspended task_2
0x10f27fc3d800 suspended task_1
0x10f27fc3c000 queued    task_0
0x10f27fc38000 suspended span
0x10f27fc42000 running   task_4
(gdb) utask list -s suspended -s queued -n 'task_\\d+'
Task ID        State     Span name
0x10f27fc40800 suspended task_3
0x10f27fc3f000 suspended task_2
0x10f27fc3d800 suspended task_1
0x10f27fc3c000 queued    task_0
(gdb) utask list -b 'some::ProblemFunction'
Task ID        State     Span name
0x10f27fc40800 suspended task_3
```

* `utask apply <task> <cmd...>`: Executes `<cmd...>` in the context of selected `<task>`. The `<task>` may be
specified by its ID ("Task") or name ("Span") (as shown in `utask list`), or set to "all" to apply the command to all
 tasks. `<cmd...>` can be any GDB command, including Python scripts. See `help utask apply` for more details.


#### Examples:

1. Print "Hello world!" for all tasks
```
(gdb) utask apply all print "Hello world!"
Executing command `print "Hello world!"` for task 0x10f27fc40800
$1 = "Hello world!"
Executing command `print "Hello world!"` for task 0x10f27fc3f000
$2 = "Hello world!"
Executing command `print "Hello world!"` for task 0x10f27fc3d800
$3 = "Hello world!"
Executing command `print "Hello world!"` for task 0x10f27fc3c000
$4 = "Hello world!"
Executing command `print "Hello world!"` for task 0x10f27fc38000
$5 = "Hello world!"
Executing command `print "Hello world!"` for task 0x10f27fc42000
$6 = "Hello world!"
```

2. Get backtrace of the suspended `task_1`
```
(gdb) utask apply task_1 backtrace
Executing command `backtrace` for task 0x10f27fc3d800
#1  0x00000000006131d8 in boost::context::fiber::resume() && (this=0x7fffefae6e38)
    at /usr/include/boost/context/include/boost/context/fiber_fcontext.hpp:377
#2  boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>::control_block::resume (this=0x7fffefae6e38)
    at /usr/include/boost/coroutine2/include/boost/coroutine2/detail/pull_control_block_cc.ipp:147
#3  0x0000000000611b2c in boost::coroutines2::detail::pull_coroutine<engine::impl::TaskContext*>::operator() (this=<optimized out>)
    at /usr/include/boost/coroutine2/include/boost/coroutine2/detail/pull_coroutine.ipp:77
#4  engine::impl::TaskContext::Sleep (this=0x10f27fc3d800, wait_strategy=..., deadline=...)
    at /home/.../userver/core/src/engine/task/task_context.cpp:332
#5  0x00000000005fbaca in engine::impl::ConditionVariableAny<engine::Mutex>::WaitUntil (this=0x7fffefb68c50, lock=..., deadline=...)
    at /home/.../userver/core/src/engine/impl/condition_variable_any.cpp:72
#6  0x00000000003f98b6 in TestMultipleCoroutines(int, int)::$_0::operator()() const::'lambda'()::operator()() const (this=0x10f27fc3eba8)
    at /home/.../userver/scripts/gdb/tests/src/cmd/utask/gdb_test_utask.cpp:158
......
```

3. Run a Python script in the context of `task_1` (see [GDB Python API](https://www.sourceware.org/gdb/current/onlinedocs/gdb.html/Python-API.html))
```
(gdb) utask apply task_1 python print('Hello from python!', 'current frame:', gdb.selected_frame().function())
Executing command `python print('Hello from python!', 'current frame:', gdb.selected_frame().function())` for task 0x10f27fc3d800
Hello from python! current frame: boost::context::fiber::resume() &&
```

For now `utask` commands are implemented for only linux x86 platforms, but can be easily extended for other platforms.

In addition, all of the above functionality works for debugging both a live process and coredumps.


@anchor stack_usage_debugging_with_gdb 
## Stack usage debugging with GDB

To debug a high stack usage just run your service under GDB, reproduce the situation with high stack consumption and
break on it.

After that the following command will show the stack space consumption by frames:

```
frame apply all -s printf "Frame stack usage is %d bytes\n", (char *)$rbp - (char *)$rsp
```

For example:
```
(gdb) frame apply all -s printf "Frame stack usage %d bytes\n", (char *)$rbp - (char *)$rsp
#0  0x00007ffff7c9bc1b in pthread_sigmask () from /lib/x86_64-linux-gnu/libc.so.6
Frame stack usage is 176 bytes
#1  0x00000000047ea295 in utils::SignalCatcher::~SignalCatcher (this=0x7fffffffd018) at userver/core/src/utils/signal_catcher.cpp:19
Frame stack usage is 16 bytes
#2  0x00000000047b92b8 in components::(anonymous namespace)::DoRun() at userver/core/src/components/run.cpp:243
Frame stack usage is 1728 bytes
#3  0x00000000047b88d1 in components::Run() at userver/core/src/components/run.cpp:253
Frame stack usage is 96 bytes
#4  0x00000000047a1326 in utils::DaemonMain (vm=..., components_list=...) at userver/core/src/utils/daemon_run.cpp:79
Frame stack usage is 896 bytes
#5  0x00000000047a0d61 in utils::DaemonMain (argc=3, argv=0x7fffffffdac8, components_list=...) at userver/core/src/utils/daemon_run.cpp:62
Frame stack usage is 448 bytes
#6  0x0000000003c30c72 in main (argc=3, argv=0x7fffffffdac8) at services/test/src/main.cpp:70
Frame stack usage is 496 bytes
```

Note that the above command may show garbage for last frames:
```
#30 0x0000000004567f3f in make_fcontext () at boost/context/src/asm/make_x86_64_sysv_elf_gas.S:149
Frame stack usage is 208822399 bytes
#31 0x0000000000000000 in ?? ()
Frame stack usage is 208822391 bytes
```


## GDB complains: received signal ?, Unknown signal

This is a side effect of the stack usage monitor interfering with GDB. In recent versions, the framework automatically detects
an attached debugger and disables the stack usage monitor to avoid this issue, printing the log warning: "Detected attached 
debugger. Stack usage monitor is disabled".

If automatic detection does not work in your environment, or if you want to disable the monitor for other reasons, you have 
several options. For unit tests, you can set the environment variable USERVER_ENABLE_STACK_USAGE_MONITOR=0. For other binaries, 
you can either disable it via the static config option coro_pool.stack_usage_monitor_enabled in 
components::ManagerControllerComponent, or disable it entirely at build time using USERVER_FEATURE_STACK_USAGE_MONITOR (see @ref
 scripts/docs/en/userver/build/options.md).

## Adding new pretty-printers and commands

If you need a new pretty-printer or a GDB command, you can always implement it yourself in `userver/scripts/gdb` and
bring us a PR!


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/profile_context_switches.md | @ref scripts/docs/en/userver/grpc/grpc.md ⇨
@htmlonly </div> @endhtmlonly
