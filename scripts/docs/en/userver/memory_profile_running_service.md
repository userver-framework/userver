# Memory profiling a production service

Memory profiling is used to detect inefficient usage of dynamic memory and
memory leaks. Some use cases for the memory profiler are:

* the service is running out of RAM (RSS), which creates problems in production
  right up to the point of the OOM killer.
* there are suspicions of a memory leak. For example, the RSS usage is
  constantly growing and does not reach a plateau until the service is
  restarted.
* there are arguments in favor of the fact that the service will soon
  experience a shortage of RSS. For example, due to the growth of caches or
  other internal structures by an order of magnitude.

All the userver based services could use
[jemalloc profiler](https://github.com/jemalloc/jemalloc/wiki/Use-Case%3A-Heap-Profiling)
that could be is dynamically enabled/disabled via the server::handlers::Jemalloc.

## How to profile a running service
1. Set the environment variable `MALLOC_CONF` for your service before the service start:
   ```
   MALLOC_CONF="prof:true,prof_active:false,lg_prof_sample:14,prof_prefix:/tmp/jeprof-SERVICE"
   ```
2. Start your service
3. To start sampling call the server::handlers::Jemalloc. If you use the
   static config from @ref scripts/docs/en/userver/tutorial/production_service.md then you
   have to do something like this:
   ```
   bash
   $ curl -X POST localhost:1188/service/jemalloc/pprof/enable
   OK
   $ curl -s -X POST localhost:1188/service/jemalloc/pprof/stat | grep ' prof.active:'
     prof.active: true
   ```
   If you see "ОК", then the sampling started.

4. Wait for samples to accumulate and @ref how-to-analyse-the-dump "analyse them".

5. Do not forget to turn off the profiling:
   ```
   $ curl -X POST localhost:1188/service/jemalloc/pprof/disable
   OK
   $ curl -s -X POST localhost:1188/service/jemalloc/pprof/stat | grep ' prof.active:'
     prof.active: false
   ```

## How to profile at service start

1. Set the environment variable `MALLOC_CONF` for your service before the service start:
   ```
   MALLOC_CONF="prof:true,prof_active:true,lg_prof_sample:14,prof_prefix:/tmp/jeprof-SERVICE"
   ```

2. Start your service

3. Ensure that the profiler is active:
   ```
   bash
   $ curl -s -X POST localhost:1188/service/jemalloc/pprof/stat | grep ' prof.active:'
     prof.active: true
   ```

4. @ref how-to-analyse-the-dump "Analyse the profile".

5. Do not forget to turn off the profiling:
   ```
   bash
   $ curl -X POST localhost:1188/service/jemalloc/pprof/disable
   OK
   $ curl -s -X POST localhost:1188/service/jemalloc/pprof/stat | grep ' prof.active:'
     prof.active: false
   ```

@anchor how-to-analyse-the-dump
## How to analyse the dump

After enabling the profiler, take a heap profile snapshot and analyse it.
The methods below are **independent**.

### Via `/heap`, without the service binary

**`/heap`** (`GET`) — the service returns the profile **in the HTTP response
body**. From your local machine you can fetch a profile from a remote service
without creating a file on it. The handler implements the `pprof` protocol:
`jeprof` requests `/heap` and asks the service to symbolize addresses itself.

**This is the recommended approach.** You do not need to haul a multi-gigabyte
binary off the build cache, and you avoid silently wrong symbols when the
deployed binary does not match the one you downloaded.

```
bash
$ jeprof --raw http://localhost:1188/service/jemalloc/pprof/heap > out.raw
$ jeprof --text out.raw
```

The second command needs neither the service nor its binary. Note that
`--lines` and `--disasm` do not work in this mode - they require a binary.

### From a dump file on the remote machine after `/dump` (service binary required)

**`/dump`** (`POST`) — jemalloc writes a dump file **on the service machine**
(the directory is set in `prof_prefix` in `MALLOC_CONF`). The file is then
analysed separately, usually with the service binary.

```
bash
$ curl -X POST localhost:1188/service/jemalloc/pprof/dump
OK
```

Use this flow when you already have a `.heap` file from the service machine
and want a PDF call graph or a text report with `--lines` / `--disasm`.

1. To decrypt the dump file, you need the binary files of your service and
   dynamic libraries with which the process was launched during profiling.
   Therefore, it is recommended to run `jeprof` on the target machine and parse
   the resulting pdf/text file on your working machine.

2. If you want to get a call graph with notes about the allocated memory, use the command:
    ```
    bash
    $ jeprof --show_bytes --pdf build/services/userver-sample /tmp/jeprof.5503.1.m1.heap> prof.pdf
    $ gnome-open prof.pdf # Open the file in PDF viewer
    ```

3. If you want to get the top of memory allocation commands, use the command:
    ```
    bash
    $ jeprof --show_bytes --text --cum build/samples/userver-sample /tmp/jeprof.5503.1.m1.heap
    Using local file build/services/userver-sample.
    Using local file /tmp/jeprof.5503.1.m1.heap.
    ```


## FAQ
- **Q:** I get `mallctl() returned error: Bad address` when calling `dump`.

  **A:** You need to make sure that the directory specified in `prof_prefix` is
     created and that the service has write access to it.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/service_monitor.md | @ref scripts/docs/en/userver/dns_control.md ⇨
@htmlonly </div> @endhtmlonly
