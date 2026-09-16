# uPg: Cluster topology discovery

@par Principles of PgaaS role determination
- Every host except master is in recovery state from PostgreSQL's POV.
This means the check 'select pg_is_in_recovery()' returns `false` for the
master and `true` for every other host type.
- Some hosts are in sync slave mode. This may be determined by executing
'show synchronous_standby_names' on the master.
See
https://www.postgresql.org/docs/current/runtime-config-replication.html#GUC-SYNCHRONOUS-STANDBY-NAMES
for more information.

@par PgaaS sync slaves lag
By default, PgaaS synchronous slaves are working with 'synchronous_commit'
set to 'remote_apply'. Therefore, sync slave may be lagging behind the
master and thus is not truly 'synchronous' from the reader's POV,
but things may change with time.

@par Implementation
Topology update runs every second.

Every host is assigned a connection with special ID (4100200300).
Using this connection we check for host availability, writability
(master detection) and perform RTT measurements.

After the initial check we know about master presence and RTT for each host.
Master host is queried about synchronous replication status. We use this
info to identify synchronous slaves and to detect "quorum commit" presence.

@par RTT-based host selection
The static `rtt_threshold` setting and its dynamic `rtt_threshold_ms`
counterpart define a latency window relative to the fastest alive host eligible
for selection. Both default to 20 milliseconds. A zero threshold is valid and
prefers only hosts tied for the lowest RTT. The
`POSTGRES_RTT_THRESHOLD_ENABLED` dynamic config defaults to `true`; disable it
to turn off RTT preference. When no selection strategy is specified, or when
`kRoundRobin` is requested, hosts whose RTT is less than or equal to the minimum
RTT plus the threshold are preferred.

RTT is tracked using the same exponentially weighted moving average as MongoDB
SDAM. The first sample initializes the estimate; subsequent samples use
`0.2 * latest_rtt + 0.8 * previous_rtt`. This estimate is used for threshold
preference, nearest selection, topology debug logs, and `roundtrip-time`
metrics.

The RTT threshold is a soft preference, not an availability check. A host
outside the latency window remains alive and keeps its detected role. If no
eligible host has a known RTT, the driver falls back to all alive hosts matching
the requested role. In particular, a slow replica is still used before the
existing replica-to-master role fallback is considered. The nearest strategy
always chooses the alive host with the lowest EWMA RTT; it does not apply the
threshold filter.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/errors.md | @ref scripts/docs/en/userver/pg/connlimit_mode_auto.md ⇨
@htmlonly </div> @endhtmlonly
