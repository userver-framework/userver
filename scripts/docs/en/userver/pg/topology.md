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


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/errors.md | @ref scripts/docs/en/userver/pg/connlimit_mode_auto.md ⇨
@htmlonly </div> @endhtmlonly
