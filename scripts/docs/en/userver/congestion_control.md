# Congestion Control

## Introduction

congestion_control::Component (aka CC) limits the active requests count. CC has a RPS (request per second) limit mechanism that turns on
and off automatically depending on the main task processor workload. In case of overload CC responds with HTTP 429 codes to some requests,
allowing your service to properly process the rest. The RPS limit is determined by a heuristic algorithm inside CC. 
All the significant parts of the component are configured by dynamic config options USERVER_RPS_CCONTROL and USERVER_RPS_CCONTROL_ENABLED.

CC can run in `fake-mode` with no RPS limit (but FSM works). CC goes into `fake-mode` in the following cases:

* there are no reliable guarantees on CPU, in this case RPS-limit would be triggered too often,
* service has no HTTP-handles others than server::handlers::Ping.

## Usage

congestion_control::Component can be useful if your service stops handling requests when overloaded, significantly increasing response time, responding with HTTP 500 codes to requests, eating memory.
Including CC in your service will help you handle some reasonable request flow returning HTTP error codes to the rest. 

### Usage restrictions

congestion_control::Component cannot be useful if:

1. Your service has an exact RPS limit requirement. The heuristic algorithm depends on how much and in what mode the CPU is used in the current container. 

2. CPU is not a limiting resource.

3. Stable response time is not needed. Some services do not need stable response time. They process events in batches which can come at random times. Despite peak overload may be reached there is enough resources to process event flow on average. For example services like PUBSUB subscriber or message broker consumer cannot benefit from a CC component.

4. CPU request processing lasts a considerable time. The RPS limit mechanism assumes that the handler processing time is short and RPC limit change affects CPU workload (seconds or less). If the handler takes a long time to process a request then the feedback will be stalling and maximum RPS convergence is not guaranteed.

## Example configuration

1. Add congestion_control::Component in the static config:
```yaml
components_manager:
    components:
        congestion-control:
            fake-mode: $testsuite-enabled
            load-enabled: true
```

2. Enable USERVER_RPS_CCONTROL_ENABLED.

3. Adjust the heuristic settings in USERVER_RPS_CCONTROL if needed.

It is a good idea to disable congestion_control::Component in unit tests to avoid getting HTTP 429 on an overloaded CI server.

## Settings

In some situations default settings are ineffective. For example:

* if serializationg / parsing request body exceeds 10 ms,

* if request processing is done without context switch more than 10 ms,

in those situations congestion_control::Component settings need adjusting. 

Basic configuration files:

* USERVER_RPS_CCONTROL_ENABLED - turning CC on and off

* USERVER_RPS_CCONTROL - basic CC settings

* USERVER_TASK_PROCESSOR_QOS - note `default-service.default-task-processor.wait_queue_overload.sensor_time_limit_us`. 
This setting defines wait in queue time after which the overload events for RPS congestion control are generated. 
It is recommended to set this setting >= 2000 (2 ms) because system scheduler (CFS) time unit by default equals 2 ms.

## Diagnostics

In case RPS mechanism is triggered it is recommended to ensure that there is no mistake. If RPS triggering coinsided 
with peak CPU comsumption than there is no mistake and the lack of resources situation needs to be resolved:

1. You need to locate slow code and optimise it for the workload.

2. Do test runs and increase the resources if needed (more memory or more kernels for each pod in a cluster).

If RPS triggering did not coinside with peak CPU comsumption than there is no lack of resources but a different kind of problem.
Most likely your service has synchronous operations that block the coroutine flow. If this is the case then you need to either:

* Try to find synchronous system calls which block the corountine flow.

* Try to find slow coroutines. To do this you can use coroutine profiling. See the dynamic configuration file USERVER_TASK_PROCESSOR_PROFILER_DEBUG 
that stores profiling settings. After finding such coroutines you need to locate trouble-making code with log analysis etc.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/deadline_propagation.md |
@ref scripts/docs/en/userver/caches.md ⇨
@htmlonly </div> @endhtmlonly
