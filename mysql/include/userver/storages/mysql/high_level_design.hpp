#pragma once

/// @file userver/storages/mysql/high_level_design.hpp
/// This file is for documentation purposes only.

/// @page mysql_driver_design
/// ## Driver Architecture
/// Like most drivers in uServer the uMySQL driver consists of three logically
/// separate layers:
/// 1. User-facing side - component, client interface, result types etc..
/// 2. Driver infrastructure - topology, connections pools, some
/// helpers/wrappers, configs/logging/metrics and so on
/// 3. The communication layer, which is responsible for an actual interaction
/// with a service a driver is built for (MySQL/MariaDB in our case)
///
/// Briefly, all these layers are glued like this:
/// 1. User-facing layer takes an input from the user, converts it into some
/// internal representation and asks the infrastructure layers to execute a
/// request.
/// 2. Infrastructure layer finds an appropriate connection and executes a
/// request on it.
/// 3. The communication layer, to which a connection belongs, converts internal
/// representation acquired at step 1. into native representation, executes a
/// request and converts native response into internal representation.
/// 4. After infrastructure layer executed a request and returned,
/// user-facing layer converts internal representation of a response into
/// whatever user requested.
///
/// There actually isn't that much to tell about infrastructure layer - it's
/// just a bunch of connections pools, defined by topology, where every pool
/// maintains some lockfree storage of connections and rents them to user-facing
/// layer. <br>
/// Communication layer and user-facing layer, however, are different
/// story, since they have to do some magic to not only make user types work out
/// of the box, but also make containers of user types work seamlessly as well.
///
/// ## Welcome MYSQL_BIND
/// `MYSQL_BIND` is a structure used by mariadbclient and mysqlclient for
/// binding prepared statement parameters/result from/to `C` types. When a
/// statement is executed for every parameter marker in it mariadbclient (i'll
/// only talk about mariadbclient here, since that is what driver uses) feeds
/// the data from corresponding `MYSQL_BIND` to a server, and when a statement
/// execution results are fetched from the server mariadbclient populates
/// `MYSQL_BIND` with data. <br>
/// These processes are different, and i'll talk about input/output parts
/// separately.
///
/// For input parameters binding `MYSQL_BIND` resembles to this:
/// @code{.cpp}
/// struct MYSQL_BIND {
///     enum_field_types buffer_type;
///     void* buffer;
///     std::size_t buffer_length;
/// };
/// @endcode
/// which is self-explanatory; <br>
/// * to bind a not engaged optional one should set `buffer_type` to
/// `MYSQL_TYPE_NULL`
/// * to bind an `int a` one should set `buffer_type` to `MYSQL_TYPE_LONG` and
/// `buffer` to `&a`, <br>
/// * to bind a `std::string str` one should set `buffer_type` to
/// `MYSQL_TYPE_STRING` and `buffer/buffer_length` to `str.data()/str.size()`
/// accordingly
/// * ...
///
/// you get the idea. For types that have no corresponding counterpart (say,
/// `std::chrono::...` or `userver::formats::Json`) one should first serialize
/// them to something mariadbclient understands and bind that serialized
/// representation. <br>
/// This is cool and all, and it should be pretty straightforward by now how to
/// specialize some templates for selected subset of types (ints, strings and so
/// on) for binding, but how does one make it work with unknown upfront
/// user-defined aggregates?
///
/// ## Welcome boost::pfr
/// And this is `some magic` part. By non other than lead maintainer of uServer
/// Antony Polukhin himself, <br>
/// `boost::pfr` library is `std::tuple like methods for user defined types
/// without any macro or boilerplate code`, which allows one to do this:
/// @code{.cpp}
/// struct MyAggregate final {
///    ...
/// };
/// constexpr auto fields_count = boost::pfr::tuple_size_v<MyAggregate>;
/// boost::pfr::for_each_field(MyAggregate{}, [](auto&& field) {});
/// @endcode
/// It has some limitations but in general "just works", and with it the driver
/// can
/// 1. Initialize an array of `MYSQL_BIND` of size needed
/// 2. For every field `F` of an aggregate `T` find an appropriate `Binder<F>`
/// class
/// 3. Bind the value of `F` into corresponding MYSQL_BIND
///
/// C++ templates are something else indeed.
///
/// ## Batched INSERT
/// MariaDB 10.2.6+ supports batched inserts and mariadbclient also supports it
/// (in general it supports an array or params in prepared statement), as one
/// could expect. <br>
/// The API is pretty straightforward: <br>
/// Generate some binds, set `STMT_ATTR_ARRAY_SIZE` to specify amount of rows,
/// set `STMT_ATTR_ROW_SIZE` to specify a row size, so mariadbclient can do its
/// pointers arithmetic, and you are good to... Hold up. <br><br>
/// `Pointers arithmetic`, right, that's the catch. Mariadbclient being `C`
/// library expects (and rightfully so) user data to be a `C` struct as well,
/// and API expects an array of these structs to be contiguous in memory,
/// so it can add `STMT_ATTR_ROW_SIZE` to `MYSQL_BIND.buffer` go get the field
/// buffer of the next row. <br>
/// We could generate a `std::vector` of tuples of conforming `C` types, but
/// that is a potentially heavy allocations and a lot of difficult to understand
/// metaprogramming, and turns out there is a better way:
/// `STMT_ATTR_CB_PARAM`.<br>
/// This is an undocumented feature (https://jira.mariadb.org/browse/CONC-348)
/// and is used in 2 repositories across all of the github (guess where,
/// `mariadb-connector-c++` and `mariadb-connector-python`), but it's there
/// since Dec 3, 2018 therefore i consider it stable enough.<br> Basically it
/// allows one to specify a callback which will be called before mariadbclient
/// copies data from MYSQL_BIND into its internals, and with it instead of
/// transforming user container into `std::vector` we can maintain a mere
/// iterator of that user container, and when prebind callback fires just
/// populate the binds and advance the iterator. <br> Voila, no heavy
/// metaprogramming (to transform user type into tuple of `C` types), no
/// intermediate allocations; a thing of beauty IMO.
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
