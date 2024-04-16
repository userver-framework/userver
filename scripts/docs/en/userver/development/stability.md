## API and ABI stability, versioning

Userver is used by hundreds of microservices only in our code base. Manual
application of modifications to the whole code-base is a burden.

**Rule of the thumb:** we endeavor to change things only if the migration path
is simple or there is a script to automate it.

## Versioning

Userver follows the two-part versioning since version `2.0`:

`major.minor`

Examples: `2.0`, `2.1_rc`


## API stability

We attempt to keep the API stable as long as it does not stand in the way of
new features or better safety.

Note that framework internals located in `*::impl::*` and `*::detail::*`
namespaces have no stability guarantees. You should not use those in your code
directly.


## Compilation

Transitive includes could not be relied on. Even a change in patch
version could remove some `#include` from header and break code that
relies on transitive includes. However, we provide script 
`./scripts/add-missing-include.sh` that helps to add missing includes
in big code bases.

Avoid adding your own forward declarations for the framework types as the type
could change to an alias or vice versa during the userver development process.


## ABI stability

Userver is meant for usage mostly as a static library that is linked with the
application. Because of that userver has no Application **Binary** Interface
(ABI) stability guarantees. Binary interface
may change on any commit.

However, you could redefine cmake options
`USERVER_NAMESPACE`, `USERVER_NAMESPACE_BEGIN` and `USERVER_NAMESPACE_END` to
place all the entities of the userver into versioned inline namespace. That
would to allow you mixing different versions of userver in a single binary
without ODR-violations.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref clickhouse_driver | @ref scripts/docs/en/userver/driver_guide.md ⇨
@htmlonly </div> @endhtmlonly
