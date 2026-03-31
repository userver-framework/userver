# Congestion Control

## Introduction

@ref congestion_control::Component (aka CC) limits the active requests count. CC has a RPS (request per second)
limit mechanism that turns on and off automatically depending on the main task processor workload. In case of overload
CC responds with HTTP 429 codes to some requests, allowing your service to properly process the rest. The RPS limit
is determined by a heuristic algorithm inside CC.  All the significant parts of the component are configured
by dynamic config options @ref USERVER_RPS_CCONTROL and @ref USERVER_RPS_CCONTROL_ENABLED.

CC can run in `fake-mode` with no RPS limit (but FSM works). CC goes into `fake-mode` in the following cases:

* there are no reliable guarantees on CPU, in this case RPS-limit would be triggered too often,
* service has no handlers others than @ref server::handlers::Ping, which has throttling disabled.

`fake-mode` can be useful for more flexible traffic restriction settings, according to it's more complex logic, which
can be implemented in a middleware.

## Usage

@ref congestion_control::Component can be useful if your service stops handling requests when overloaded, significantly
increasing response time, responding with HTTP 500 codes to requests, eating memory.

Including CC in your service will help you handle some reasonable request flow returning HTTP error codes to the rest.

### Usage restrictions

@ref congestion_control::Component cannot be useful if:

1. Your service has an exact RPS limit requirement. The heuristic algorithm depends on how much and in what mode the
   CPU is used in the current container.

2. CPU is not a limiting resource.

3. Stable response time is not needed. Some services do not need stable response time. They process events in batches
   which can come at random times. Despite peak overload may be reached there is enough resources to process event flow
   on average. For example services like PUBSUB subscriber or message broker consumer cannot benefit from a
   CC component.

4. CPU request processing lasts a considerable time. The RPS limit mechanism assumes that the handler processing time
   is short and RPC limit change affects CPU workload (seconds or less). If the handler takes a long time to process
   a request then the feedback will be stalling and maximum RPS convergence is not guaranteed.

## Example configuration

1. Add @ref congestion_control::Component in the static config:
```
# yaml
components_manager:
    components:
        congestion-control:
            fake-mode: $testsuite-enabled
            load-enabled: true
```

2. Set to `true` the value of @ref USERVER_RPS_CCONTROL_ENABLED dynamic config.

3. Adjust the heuristic settings in @ref USERVER_RPS_CCONTROL if needed.

It is a good idea to disable @ref congestion_control::Component in unit tests to avoid getting HTTP 429 on an
overloaded CI server.

### Detailed Description of the Congestion Control Logic

The Congestion Control logic is implemented as sensors (`overloads_ps`, `rps`) and a state machine with variables.

**Sensor Data:**
- `overloads_ps` – Number of tasks in the last second that waited in the execution queue for more than
`USERVER_TASK_PROCESSOR_QOS.default-service.default-task-processor.sensor_time_limit_us` microseconds (default: 3ms).
- `rps` – Number of requests received in the last second.

**State Machine Variables:**
- `is_overloaded – Whether the server is stably under unsustainable load.
- `is_overloaded_now` – Whether the server is currently under unsustainable load.
- `current_limit` – Current RPS limit.

The congestion control state machine has 5 states:
- no `current_limit`
- `is_overloaded=true`, `is_overloaded_now=true` – The service is currently overloaded and has been overloaded for a
  long time. Congestion Control **decreases service RPS limit by `down_rate_percent`%**.
- `is_overloaded=true`, `is_overloaded_now=false` – The service is not overloaded now but was overloaded very recently.
- `is_overloaded=false`, `is_overloaded_now=true` – The service is currently overloaded but was minimally overloaded
  recently.
- `is_overloaded=false`, `is_overloaded_now=false` – The service is not overloaded now and was minimally overloaded
  recently. Congestion Control **increase service RPS limit by up_rate_percent%**.

@dot
digraph A {
  no_limit [shape = "roundedbox"];
  is_now [shape = "roundedbox", label="is_overloaded\nis_overloaded_now\n(RPS limit decreases)"];
  no_is_no_now [shape = "roundedbox", label="!is_overloaded\n!is_overloaded_now\n(RPS limit increases)"];
  no_is_now [shape = "roundedbox", label="!is_overloaded\nis_overloaded_now"];
  is_no_now  [shape = "roundedbox", label="is_overloaded\n!is_overloaded_now"];

  no_limit -> is_now;
  no_is_no_now -> no_limit;
  is_no_now -> is_now [minlen=5];
  no_is_no_now -> no_is_now [minlen=5];
  {rank=same is_now -> is_no_now}
  is_no_now -> no_is_no_now;
  no_is_now -> is_now;
  {rank=same no_is_now -> no_is_no_now}
}
@enddot

**Semantics of the `is_overloaded` flag:**
- If `is_overloaded=true`, the service either decreases the RPS limit or waits (depending on `is_overloaded_now`).
- If `is_overloaded=false`, the service either increases the RPS limit or waits (depending on `is_overloaded_now`).

**Key State Transitions:**
- Transition **is_overloaded=true, is_overloaded_now=false → is_overloaded=false, is_overloaded_now=false** occurs if
  `overloads_ps <= down-level` holds for `overload-off` seconds (i.e., the service experienced no overload for
  `overload-off` consecutive seconds).
- Transition **is_overloaded=false, is_overloaded_now=true → is_overloaded=true, is_overloaded_now=true** occurs if
  `overloads_ps > up-level` holds for `overload-on` seconds (i.e., the service experienced overload for `overload-on`
  consecutive seconds).

**Transitions between is_overloaded_now states:**
- If `is_overloaded=true`, compute `is_overloaded_now = overloads_ps > down-level`.
- If `is_overloaded=false`, compute `is_overloaded_now = overloads_ps > up-level`.

**Purpose of splitting is_overloaded and is_overloaded_now:**
This design mitigates flapping. Services may frequently execute isolated long tasks that monopolize the task processor
for significant periods without critically impacting performance (wait times are negligible). Simultaneously, it
ensures progressive limit increases when many tasks experience low wait times.


## Settings

In some situations default settings are ineffective. For example:

* if serializationg / parsing request body exceeds 10 ms,

* if request processing is done without context switch more than 10 ms,

in those situations congestion_control::Component settings need adjusting.

Basic dynamic configuration options:

* @ref USERVER_RPS_CCONTROL_ENABLED - turning CC on and off

* @ref USERVER_RPS_CCONTROL - basic CC settings

* @ref USERVER_TASK_PROCESSOR_QOS - note
  `default-service.default-task-processor.wait_queue_overload.sensor_time_limit_us`.
  This setting defines wait in queue time after which the overload events for RPS congestion control are generated.
  It is recommended to set this setting >= 2000 (2 ms) because system scheduler (CFS) time unit by default equals 2 ms.

## Diagnostics

In case RPS mechanism is triggered it is recommended to ensure that there is no mistake. If RPS triggering coincided
with peak CPU consumption than there is no mistake and the lack of resources situation needs to be resolved:

1. You need to locate slow code and optimise it for the workload.

2. Do test runs and increase the resources if needed (more memory or more kernels for each pod in a cluster).

If RPS triggering did not coincide with peak CPU consumption than there is no lack of resources but a different kind of problem.
Most likely your service has synchronous operations that block the coroutine flow. If this is the case then you need to either:

* Try to find synchronous system calls which block the corountine flow.

* Try to find slow coroutines. To do this you can use coroutine profiling. See the dynamic configuration
  @ref USERVER_TASK_PROCESSOR_PROFILER_DEBUG that stores profiling settings. After finding such coroutines you need
  to locate trouble-making code with log analysis etc.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/deadline_propagation.md |
@ref scripts/docs/en/userver/stack.md ⇨
@htmlonly </div> @endhtmlonly
