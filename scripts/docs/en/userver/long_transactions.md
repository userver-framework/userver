# Long Transactions Detection

When working with databases, 🐙 userver provides the ability to create a transaction in code and execute database
queries within it. From creation to commit/rollback, a transaction holds one of the database connections entirely.
If a heavy operation is called during the transaction’s lifetime (e.g., an HTTP request), then in case of an incident
(e.g., the service being called via HTTP goes down), the execution time of that operation will increase.
Consequently, the transaction’s lifetime will also increase while the connection is hold, which may cause the entire
database to fail.

Consequently there is a built-in mechanism for long transactions detection. See also @ref utils::trx_tracker.


## Drawbacks of Long Transactions

* **Exhaustion of the connection pool**. Each transaction occupies a database connection for its entire duration.
  Prolonged transactions tie up more connections, risking exhaustion of the connection pool.
* **Exhaustion of database resources**. To ensure transaction isolation between data writing and completion, the
  database must maintain two versions of each row (old and new). With many concurrent transactions, the number of row
  duplicates grows significantly, leading to substantial increases in table storage usage.
* **Execution linearization**. Only one operation can modify a specific database row at a time; others must wait.
  If a writing transaction becomes blocked, all subsequent write operations are also halt prepared to proceed.


## Where the Long Transactions Detection check works

To detect long operations within transactions, we insert a check for active transactions into every heavy operation
in userver. When triggered, this check writes a log and a metric.

The check is inserted into:

+ HTTP requests:
  + Synchronous code‑generated requests;
  + Waiting for asynchronous code‑generated requests;
  + Working with STQ (CallLater, Reschedule, ...);
+ gRPC requests.

Transactions from the following databases are monitored:

+ MySQL;
+ PostgreSQL;
+ SQLite;
+ YDB.

The check works in testsuite tests but is disabled in gtest and gbenchmark.


## How the Long Transactions Detection check failure is displayed?

**Logs**:

When the check is triggered, the service will spam `WARN` logs with the following text:
`Long call while having active transactions`. The `meta.location` field displays the name of the method that calls the
endpoint where the check was triggered.

**Metrics**:

Metric: `engine.heavy-operations-in-transactions` is incremented in case of heavy operation in transaction.


## How to fix the Long Transactions Detection check failure

Move the heavy request out from the transaction.

If the bahavior is understood and it is fine to have a long transaction in that particular case, the check could be
disabled for a scope via @ref utils::trx_tracker::CheckDisabler :

@snippet utils/trx_tracker_test.cpp Sample CheckDisabler usage

To disable the check for the whole service use the `enable_trx_tracker` static configuration option of
@ref components::ManagerControllerComponent. Note that this is **not** recommended.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/dump_coroutines.md | @ref scripts/docs/en/userver/caches.md ⇨
@htmlonly </div> @endhtmlonly

