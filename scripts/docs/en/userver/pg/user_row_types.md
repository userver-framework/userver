# uPg: Typed PostgreSQL results

The ResultSet provides access to a generic PostgreSQL result buffer wrapper
with access to individual column buffers and means to parse the buffers into
a certain type.

For a user that wishes to get the results in a form of a sequence or a
container of C++ tuples or structures, the driver provides a way to coerce
the generic result set into a typed result set or a container of tuples or
structures that fulfill certain conditions.

TypedResultSet provides container interface for typed result rows for
iteration or random access without converting all the result set at once.
The iterators in the TypedResultSet satisfy requirements for a constant
RandomAccessIterator with the exception of dereferencing iterators.

@warning The operator* of the iterators returns value (not a reference to
it) and the iterators don't have the operator->.

@par Data row extraction

The data rows can be obtained as:
  - std::tuple;
  - aggregate class as is;
  - non-aggregate class with some augmentation;
  - a subset of any of those, or a reordering of it, through a structure view.

Data members of the tuple or the classes must be supported by the driver.
For more information on supported data types please see
@ref scripts/docs/en/userver/pg/types.md.

@par std::tuple.

The first option is to convert ResultSet's row to std::tuples.

```
using MyRowType = std::tuple<int, string>;
auto trx = ...;
auto generic_result = trx.Execute("select a, b from my_table");
auto iteration = generic_result.AsSetOf<MyRowType>();
for (auto row : iteration) {
  static_assert(std::is_same_v<decltype(row), MyRowType>,
      "Iterate over tuples");
  auto [a, b] = row;
  std::cout << "a = " << a << "; b = " << b << "\n";
}

auto data = geric_result.AsContainer<std::vector<MyRowType>>();
```

@par Aggregate classes.

A data row can be coerced to an aggregate class.

An aggregate class (C++03 8.5.1 §1) is a class that with no base classes, no
protected or private non-static data members, no user-declared constructors
and no virtual functions.

```
struct MyRowType {
  int a;
  std::string b;
};
auto generic_result = trx.Execute("select a, b from my_table");
auto iteration = generic_result.AsSetOf<MyRowType>();
for (auto row : iteration) {
  static_assert(std::is_same_v<decltype(row), MyRowType>,
      "Iterate over aggregate classes");
  std::cout << "a = " << row.a << "; b = " << row.b << "\n";
}

auto data = geric_result.AsContainer<std::vector<MyRowType>>();
```

@par Non-aggregate classes.

Classes that do not satisfy the aggregate class requirements can be used
to be created from data rows by providing additional `Introspect` non-static
member function. The function should return a tuple of references to
member data fields. The class must be default constructible.

```
class MyRowType {
 private:
  int a_;
  std::string b_;
 public:
  MyRowType() = default; // default ctor is required
  explicit MyRowType(int x);

  auto Introspect() {
    return std::tie(a_, b_);
  }
  int GetA() const;
  const std::string& GetB() const;
};

auto generic_result = trx.Execute("select a, b from my_table");
auto iteration = generic_result.AsSetOf<MyRowType>();
for (auto row : iteration) {
  static_assert(std::is_same_v<decltype(row), MyRowType>,
      "Iterate over non-aggregate classes");
  std::cout << "a = " << row.GetA() << "; b = " << row.GetB() << "\n";
}

auto data = geric_result.AsContainer<std::vector<MyRowType>>();
```
@par Structure views: a subset of a structure, in query order

A row does not always match a whole structure. The query may select some of the
columns, or select them in an order the structure does not declare, or the
structure's layout may not be yours to change — a generated DTO's member order
is its schema's property. Extraction to an aggregate binds column *n* to member
*n*, so in those cases it cannot be used, and the fallback is reading the row
field by field.

storages::postgres::io::StructView names the members to bind, in query order,
and leaves the rest of the structure alone. It is declared in
`userver/storages/postgres/io/struct_view.hpp`.

```
struct User {
  boost::uuids::uuid id;
  std::string name;
  std::string email;
};

namespace pg = storages::postgres;

// Every member, in declaration order: the same thing as extracting User itself.
using FullUser = pg::io::StructView<User>;

// The same three members, in the order a particular query selects them.
using UserByEmail = pg::io::StructView<User, &User::id, &User::email, &User::name>;

// A subset: the query selects two of the three columns.
using UserNoEmail = pg::io::StructView<User, &User::id, &User::name>;

static_assert(UserNoEmail::size == 2);
```

The pack must name non-static data members of the viewed class; naming a member
of another class is a compile error.

@par Reading through a view

When reading, a view holds no data — it is a static discriminator, and the
extraction yields the underlying structure.

```
auto generic_result = trx.Execute("select id, name from users");

// A single row becomes a User.
auto user = generic_result.AsSingleRow<UserNoEmail>(pg::kRowTag);

// So does every row of a set.
for (auto row : generic_result.AsSetOf<UserNoEmail>(pg::kRowTag)) {
  static_assert(std::is_same_v<decltype(row), User>,
      "A view extracts the structure it views");
}

// A container of views is a container of the underlying structure: the
// container template is rebound, allocator and all.
auto users = generic_result.AsContainer<std::vector<UserNoEmail>>(pg::kRowTag);
static_assert(std::is_same_v<decltype(users), std::vector<User>>);
```

Members the view does not name are left value-initialised, so `email` above is
an empty string rather than anything the row carried. The viewed class must be
default constructible for that reason.

@warning A view always needs the kRowTag. Without it the call is the
single-column extraction described below. A view cannot be read as a field
value at all, and says so: `Struct views are not supported for reading as field
values`.

@par Writing through a view

The same view also sends a structure's members as a statement's parameters.
That is part of running a query rather than of processing its result, so it is
documented in @ref scripts/docs/en/userver/pg/run_queries.md.

@par Single-column result set

A single-column result set can be used to extract directly to the column
type. User types mapped to PostgreSQL will work as well. If you need to
extract the whole row into such a structure, you will need to disambiguate
the call with the kRowTag.

@code
auto string_set = generic_result.AsSetOf<std::string>();
std::string s = string_set[0];

auto string_vec = generic_result.AsContainer<std::vector<std::string>>();

// Extract first column into the composite type
auto foo_set = generic_result.AsSetOf<FooBar>();
auto foo_vec = generic_result.AsContainer<std::vector<FooBar>>();

// Extract the whole row, disambiguation
auto foo_set = generic_result.AsSetOf<FooBar>(kRowTag);

@endcode


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/types.md | @ref scripts/docs/en/userver/pg/errors.md ⇨
@htmlonly </div> @endhtmlonly
