# Fixture markers

@ref pytest_userver.fixture_markers lets plugins and tests attach typed metadata
to fixture functions and discover marked fixtures visible to a pytest request.
The implementation belongs to userver and only depends on pytest fixture
definitions; it does not require a running service.

For the common userver use cases, prefer the existing decorators:

- @ref pytest_userver.config.patch marks a fixture returning a function that
  changes the service's static configuration before startup.
- @ref pytest_userver.service.dependency marks a fixture to initialize before
  starting the service.

For custom discovery, such as database initializers or mock setup, use
@ref pytest_userver.fixture_markers.mark and
@ref pytest_userver.fixture_markers.get_infos directly as described below.
See also @ref scripts/docs/en/userver/functional_testing.md.

@anchor fixture_markers_marking
## Marking a fixture

Apply the mark to the original function, below the pytest fixture decorator.
@ref pytest_userver.fixture_markers.mark returns the same function without
executing or wrapping it. Applying it above the fixture decorator is an error.

Each exact metadata type can be attached only once to a function; a duplicate
raises `ValueError`. Distinct metadata types can coexist on the same function.
Metadata subclasses are independent types, not matches for their base class.

The following example defines metadata and a decorator for initialization
priority. To keep the example self-contained, the fixtures record initialized
table names in a list rather than connecting to a database.

@snippet testsuite/tests/fixture_markers/test_documentation.py fixture markers custom mark

Both ordinary and explicitly named fixtures can carry the mark:

@snippet testsuite/tests/fixture_markers/test_documentation.py fixture markers definitions

@anchor fixture_markers_discovery
## Querying marks

@ref pytest_userver.fixture_markers.get_infos returns a dictionary mapping
**effective pytest fixture names** to metadata, including fixture aliases.
It includes fixtures that:

- are visible to the requesting test: a plugin, an ancestor conftest, the test
  module, or its class; unrelated modules, sibling classes, and child conftests
  are excluded;
- carry or inherit metadata of the **exact requested type**;
- have a winning definition whose scope is at least as wide as the request.
  In increasing scope order: function, class, module, package, session.

Discovery does not execute fixtures. Fixtures need not appear among the test's
arguments to be discovered. The consumer explicitly requests their values when
needed. Each query returns a fresh dictionary; changing its entries does not
change stored metadata. Metadata objects themselves are not deep-copied.

@snippet testsuite/tests/fixture_markers/test_documentation.py fixture markers discovery

The explicit priority sort in this example is a consumer policy, not behavior of
@ref pytest_userver.fixture_markers.get_infos. Without that sort, the dictionary
uses the @ref fixture_markers_ordering "definition order described below".

@anchor fixture_markers_overrides
## Overriding a marked fixture

An override **inherits the mark**. There is no need to repeat
@ref pytest_userver.config.patch, @ref pytest_userver.service.dependency, or a
custom marker decorator on an overriding fixture.

@ref pytest_userver.fixture_markers.get_infos examines the definition pytest
would call. If it has no metadata of the requested type, discovery searches the
override chain from nearest to farthest and uses the first matching mark.
Metadata inheritance is independent for each type.

The following test-class override extends the marked module fixture above.
The same rule applies when a module fixture overrides a conftest or plugin
fixture:

@snippet testsuite/tests/fixture_markers/test_documentation.py fixture markers inherited override

Requesting the original fixture is optional. An override may replace its behavior
entirely and still inherit the mark. Normal pytest lookup always executes the
winning override, not the original implementation merely because it owns the
metadata.

A mark on the override **replaces metadata of that type**. Other metadata types
remain inherited. The following override replaces both metadata and fixture
behavior, while keeping its original dictionary position:

@snippet testsuite/tests/fixture_markers/test_documentation.py fixture markers replaced override

@note An unmarked override cannot remove an inherited mark. There is no unmark
operation. Narrowing the override's scope can exclude it from a wider-scoped
request, but does not remove its metadata.

@anchor fixture_markers_ordering
## Ordering guarantees

The result of @ref pytest_userver.fixture_markers.get_infos is ordered:

1. Definition sites follow pytest visibility: non-conftest plugins, ancestor
   conftests from outer to inner, the test module, then the test class.
2. Within a module, fixtures follow definition/decorator order, not alphabetical
   fixture-name order. Non-conftest plugins follow registration order.
3. An override retains the position of the **first marked definition** of the
   requested metadata type. Repeating a marker to replace metadata does not move
   that fixture to the overriding module's or class's position.

Thus metadata can come from the nearest marked override while the ordering
position comes from the original marked definition. A fixture name appears only
once in the result.

@anchor fixture_markers_userver
## Using markers with userver

@ref pytest_userver.config.patch and @ref pytest_userver.service.dependency use
these inheritance and discovery rules. Both must be applied below the pytest
fixture decorator. A config-patch fixture returns a callable accepting the
static config and config variables; a service-dependency fixture performs its
setup when requested before service startup.

Legacy config-hook lists and
@ref pytest_userver.plugins.service.extra_client_deps remain supported. This API
does not require bulk migration of existing fixtures. In particular, an override
of a fixture registered **only through a legacy hook list** is not an example of
marker inheritance: legacy registration and marker discovery are distinct.

## API reference

- @ref pytest_userver.fixture_markers.mark
- @ref pytest_userver.fixture_markers.get_infos
- @ref pytest_userver.config.patch
- @ref pytest_userver.service.dependency

----

Adapted from the upstream
[testsuite fixture-marker guide](https://github.com/yandex/yandex-taxi-testsuite/blob/develop/docs/fixture_markers.rst)
for userver's implementation and Doxygen documentation.
