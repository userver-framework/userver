// File to document groups - entities not tied to a particular file
// NOTE: You can not have a function in multiple groups!

// clang-format off

/// @defgroup userver_base_classes Base Classes
///
/// @brief Base classes that simplify implementation of functionality.


/// @defgroup userver_components Components
///
/// @brief Components that could be used
/// with utils::DaemonMain like functions; see
/// @ref scripts/docs/en/userver/component_system.md for an intro.
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref userver_clients | @ref scripts/docs/en/userver/synchronization.md ⇨
/// @htmlonly </div> @endhtmlonly


/// @defgroup userver_clients Clients
///
/// @brief Clients are classes that provide interfaces for requesting and
/// retrieving data usually from remove server.
///
/// All the clients are asynchronous. In other words, a request suspends the
/// current engine::Task and other coroutines are processed on the task
/// processor. The suspended task resumes execution on the task processor after
/// the data was retrieved.
///
/// It is a common practice to return references or smart pointers to clients
/// from a component. In such cases:
/// * a client lives as long as the component is alive
/// * and it is safe to invoke member functions of the same client instance
///   concurrently if not stated the opposite.
///
/// @see @ref scripts/docs/en/userver/component_system.md for information on
/// how to configure clients
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref scripts/docs/en/userver/component_system.md | @ref userver_http_handlers ⇨
/// @htmlonly </div> @endhtmlonly


/// @defgroup userver_http_handlers HTTP Handlers
/// @ingroup userver_components
///
/// @brief Handlers are @ref userver_components "components" that could be used
/// by components::Server to process the incoming requests.
///
/// All the HTTP handlers derive from server::handlers::HttpHandlerBase and
/// override its methods to provide functionality.
///
/// See @ref userver_components "Userver Components" for more information on
/// how to register components.
///
/// ## Configuration
/// All the handlers inherit configuration options from their base classes:
/// * server::handlers::HandlerBase
/// * server::handlers::HttpHandlerBase
///
/// All the components, including handlers, also have the
/// @ref userver_components "load-enabled" option.
///
/// ## Static configuration example:
/// Here's an example of the full config for the server::handlers::Ping handle.
/// @snippet components/common_server_component_list_test.cpp  Sample handler ping component config
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref userver_clients | @ref userver_components ⇨
/// @htmlonly </div> @endhtmlonly


/// @defgroup userver_concurrency Concurrency
/// @brief Task construction and @ref scripts/docs/en/userver/synchronization.md "synchronization primitives for coroutines"

/// @defgroup userver_universal Universal
///
/// @brief Functions and classes that could be used with or without coroutine
/// environment.
///
/// Those classes and functions reside in CMake target `userver-universal`,
/// that is a base target for all the `userver-*` targets, so all
/// these the classes and functions are available while using any of the userver
/// parts.
///
/// @see @ref scripts/docs/en/userver/tutorial/json_to_yaml.md for a usage
/// example.

/// @defgroup userver_containers Containers
///
/// @brief Utility containers

/// @defgroup userver_formats Formats
/// @brief Different containers that @ref scripts/docs/en/userver/formats.md "share the same interface and manipulate tree-like formats"

/// @defgroup userver_formats_serialize Serializers
/// @brief Files with serializers @ref scripts/docs/en/userver/formats.md "into formats"
/// @ingroup userver_formats

/// @defgroup userver_formats_serialize_sax SAX Serializers
/// @brief Files with SAX serializers @ref scripts/docs/en/userver/formats.md "into formats"
/// @ingroup userver_formats

/// @defgroup userver_formats_parse Parsers
/// @brief Files with parsers @ref scripts/docs/en/userver/formats.md "from formats"
/// @ingroup userver_formats


/// @defgroup userver_postgres_parse_and_format Postgres parsers and formatters
///
/// @brief Files with Postgres parsers and formatters

/// @defgroup userver_dump_read_write Readers and writers for dump::Dumper
///
/// @brief Files with dump readers and writers, used mostly by the caches

/// @defgroup userver_clickhouse_types Clickhouse types
///
/// @brief Files with implemented ClickHouse types

/// @defgroup userver_testsuite Testsuite Python support
///
/// @brief Functions, fixtures and classes to do
/// @ref scripts/docs/en/userver/functional_testing.md "functional testing with testsuite".

/// @defgroup userver_testsuite_fixtures Fixtures
/// @brief Fixtures to use with testsuite
/// @ingroup userver_testsuite

/// @defgroup userver_component_names Component Default Names
/// @ingroup userver_components
/// @brief Default names of components that are used in static config files

// clang-format on
