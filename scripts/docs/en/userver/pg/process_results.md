# uPg: Working with result sets

A result set returned from Execute function is a thin read only wrapper
around the libpq result. It can be copied around as it contains only a
smart pointer to the underlying result set.

The result set's lifetime is not limited by the transaction in which it was
created. In can be used after the transaction is committed or rolled back.

@par Iterating result set's rows

The ResultSet provides interface for range-based iteration over its rows.
@code
auto result = trx.Execute("select foo, bar from foobar");
for (auto row : result) {
  // Process row data here
}
@endcode

Also rows can be accessed via indexing operators.
@code
auto result = trx.Execute("select foo, bar from foobar");
for (auto idx = 0; idx < result.Size(); ++idx) {
  auto row = result[idx];
  // process row data here
}
@endcode

@par Accessing fields in a row

Fields in a row can be accessed by their index, by field name and can be
iterated over. Invalid index or name will throw an exception.
@code
auto f1 = row[0];
auto f2 = row["foo"];
auto f3 = row[1];
auto f4 = row["bar"];

for (auto f : row) {
  // Process field here
}
@endcode

@par Extracting field's data to variables

A Field object provides an interface to convert underlying buffer to a
C++ variable of supported type. Please see
@ref scripts/docs/en/userver/pg/types.md for more information on supported
types.

Functions Field::As and Field::To can throw an exception if the field
value is `null`. Their Field::Coalesce counterparts instead set the result
to default value.

All data extraction functions can throw parsing errors (descendants of
ResultSetError).

@code
auto foo = row["foo"].As<int>();
auto bar = row["bar"].As<std::string>();

foo = row["foo"].Coalesce(42);
// There is no parser for char*, so a string object must be passed here.
bar = row["bar"].Coalesce(std::string{"bar"});

row["foo"].To(foo);
row["bar"].To(bar);

row["foo"].Coalesce(foo, 42);
// The type is deduced by the first argument, so the second will be also
// treated as std::string
row["bar"].Coalesce(bar, "baz");
@endcode

@par Extracting data directly from a Row object

Data can be extracted straight from a Row object to a pack or a tuple of
user variables. The number of user variables cannot exceed the number of
fields in the result. If it does, an exception will be thrown.

When used without additional parameters, the field values are extracted
in the order of their appearance.

When a subset of the fields is needed, the fields can be specified by their
indexes or names.

Row's data extraction functions throw exceptions as the field extraction
functions. Also a FieldIndexOutOfBounds or FieldNameDoesntExist can be
thrown.

Statements that return user-defined PostgreSQL type may be called as
returning either one-column row with the whole type in it or as multi-column
row with every column representing a field in the type. For the purpose of
disambiguation, kRowTag may be used.

When a first column is extracted, it is expected that the result set
contains the only column, otherwise an exception will be thrown.

@code
auto [foo, bar] = row.As<int, std::string>();
row.To(foo, bar);

auto [bar, foo] = row.As<std::string, int>({1, 0});
row.To({1, 0}, bar, foo);

auto [bar, foo] = row.As<std::string, int>({"bar", "foo"});
row.To({"bar", "foo"}, bar, foo);

// extract the whole row into a row-type structure.
// The FooBar type must not have the C++ to PostgreSQL mapping in this case
auto foobar = row.As<FooBar>();
row.To(foobar);
// If the FooBar type does have the mapping, the function call must be
// disambiguated.
foobar = row.As<FooBar>(kRowTag);
row.To(foobar, kRowTag);
@endcode

@par Converting a Row to a user row type

A row can be converted to a user type (tuple, structure, class), for more
information on data type requirements see @ref scripts/docs/en/userver/pg/user_row_types.md

@todo Interface for converting rows to arbitrary user types

@par Converting ResultSet to a result set with user row types

A result set can be represented as a set of user row types or extracted to
a container. For more information see @ref scripts/docs/en/userver/pg/user_row_types.md

@todo Interface for copying a ResultSet to an output iterator.

@par Non-select query results

@todo Process non-select result and provide interface. Do the docs.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/run_queries.md | @ref scripts/docs/en/userver/pg/types.md ⇨
@htmlonly </div> @endhtmlonly
