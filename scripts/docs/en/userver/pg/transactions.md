# uPg: Transactions

All queries that are run on a PostgreSQL cluster are executed inside
a transaction, even if a single-query interface is used.

A uPg transaction can be started using all isolation levels and modes
supported by PostgreSQL server as specified in documentation here
https://www.postgresql.org/docs/current/static/sql-set-transaction.html.
When starting a transaction, the options are specified using
TransactionOptions structure.

For convenience and improvement of readability there are constants
defined: Transaction::RW, Transaction::RO and Transaction::Deferrable.

@see TransactionOptions

Transaction object ensures that a transaction started in a PostgreSQL
connection will be either committed or rolled back and the connection
will returned back to a connection pool.

@todo Code snippet with transaction starting and committing

Next: @ref scripts/docs/en/userver/pg/run_queries.md

See also: @ref scripts/docs/en/userver/pg/process_results.md

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/driver.md | @ref scripts/docs/en/userver/pg/run_queries.md ⇨
@htmlonly </div> @endhtmlonly
