## API and ABI stability, versioning

Userver is used by hundreds of microservices only in our code base. Manual
application of modifications to the whole code-base is a burden.

**Rule of the thumb:** we endeavor to change things only if the migration path
is simple or there is a script to automate it.

## Versioning

Userver follows the classical three-part versioning:

`major.minor.patch`

Example: `1.0.0`


## API stability

API does not change between minor and patch version changes.

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
(ABI) stability guarantees. While the API is kept stable, binary interface
may change even on patch version change.

However, you could redefine cmake options
`USERVER_NAMESPACE`, `USERVER_NAMESPACE_BEGIN` and `USERVER_NAMESPACE_END` to
place all the entities of the userver into versioned inline namespace. That
would to allow you mixing different versions of userver in a single binary
without ODR-violations.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref clickhouse_driver | @ref md_en_userver_development_releases ⇨
@htmlonly </div> @endhtmlonly
