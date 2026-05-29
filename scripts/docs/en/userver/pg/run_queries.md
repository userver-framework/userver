# uPg: Running queries

All queries are executed through a transaction object, event when being
executed through singe-query interface, so here only executing queries
with transaction will be covered. Single-query interface is basically
the same except for additional options.

uPg provides means to execute text queries only. There is no query
generation, but can be used by other tools to execute SQL queries.

@warning A query must contain a single query, multiple statements delimited
by ';' are not supported.

All queries are parsed and prepared during the first invocation and are
executed as prepared statements afterwards.

Any query execution can throw an exception. Please see @ref scripts/docs/en/userver/pg/errors.md
for more information on possible errors.

@par Queries without parameters

Executing a query without any parameters is rather straightforward.
@code
auto trx = cluster->Begin(/* transaction options */);
auto res = trx.Execute("select foo, bar from foobar");
trx.Commit();
@endcode

The cluster also provides interface for single queries
@code
#include <service/sql_queries.hpp>

auto res = cluster->Execute(/* transaction options */, sql::kMyQuery);
@endcode

You may store SQL queries in separate `.sql` files and access them via
sql_queries.hpp include header. See @ref scripts/docs/en/userver/sql_files.md
for more information.

@par Queries with parameters

uPg supports SQL dollar notation for parameter placeholders. The statement
is prepared at first execution and then only arguments for a query is sent
to the server.

A parameter can be of any type that is supported by the driver.
See @ref scripts/docs/en/userver/pg/types.md for more information.

@code
auto trx = cluster->Begin(/* transaction options */);
auto res = trx.Execute(
    "select foo, bar from foobar where foo > $1 and bar = $2", 42, "baz");
trx.Commit();
@endcode

@note You may write a query in `.sql` file and generate a header file with Query from it.
      See @ref scripts/docs/en/userver/sql_files.md for more information.
@see Transaction
@see ResultSet

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/transactions.md | @ref scripts/docs/en/userver/pg/process_results.md ⇨
@htmlonly </div> @endhtmlonly
