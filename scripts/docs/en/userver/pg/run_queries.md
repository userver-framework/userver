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

@par Passing a structure as the parameters

A statement's parameters are often the members of a structure already at hand.
storages::postgres::io::StructView sends them without naming each one at the
call site: one parameter per member it views, in the order it names them.
It is declared in `userver/storages/postgres/io/struct_view.hpp`.

@code
struct User {
  boost::uuids::uuid id;
  std::string name;
  std::string email;
};

namespace pg = storages::postgres;

User user{/* ... */};

// Every member, in declaration order: three parameters.
trx.Execute("insert into users(id, name, email) values($1, $2, $3)",
            pg::io::StructView<User>{user});

// A subset: two parameters, and the member the view does not name is not sent.
trx.Execute("insert into users(id, name) values($1, $2)",
            pg::io::StructView<User, &User::id, &User::name>{user});

// MakeStructView deduces the structure from the argument, so only the members
// have to be written out.
trx.Execute("insert into users(id, email) values($1, $2)",
            pg::io::MakeStructView<&User::id, &User::email>(user));
@endcode

The pack is the order the parameters go out in, not the structure's layout, so
a statement whose columns are ordered differently needs no shuffling by the
caller:

@code
trx.Execute("insert into users(id, email, name) values($1, $2, $3)",
            pg::io::MakeStructView<&User::id, &User::email, &User::name>(user));
@endcode

A view is accepted wherever a statement takes parameters —
storages::postgres::Transaction, storages::postgres::Cluster and a
non-transactional execution alike.

@warning A view is the whole parameter list, not one entry in it: it cannot be
combined with further arguments in the same call, and attempting it fails with
`Type doesn't have mapping to Postgres type.`, which does not name the view as
the cause. Build a view that covers every parameter instead.

@warning A view borrows the structure rather than copying it, so it must not
outlive the object it names. Constructing it in the call, as above, is the
intended shape.

@see @ref scripts/docs/en/userver/pg/user_row_types.md for reading a row back
     through the same view.

@note You may write a query in `.sql` file and generate a header file with Query from it.
      See @ref scripts/docs/en/userver/sql_files.md for more information.
@see Transaction
@see ResultSet

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/transactions.md | @ref scripts/docs/en/userver/pg/process_results.md ⇨
@htmlonly </div> @endhtmlonly
