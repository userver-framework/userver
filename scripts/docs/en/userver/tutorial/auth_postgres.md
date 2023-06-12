## Custom Authorization/Authentication via PostgreSQL

## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref md_en_userver_tutorial_hello_service.


## Step by step guide

This tutorial shows how to create a custom authorization check by tokens with
scopes for HTTP handlers. Required scopes are specified in static configuration
file for each HTTP handler. In
the tutorial the authorization data is cached in components::PostgreCache, and
information of an authorized user (it's name) is passed to the HTTP handler.

Creation of tokens and user registration is out of scope of this tutorial.

@warning Authorization scheme from this sample is vulnerable to the MITM-attack
  unless you are using a secure connection (HTTPS).

### PostgreSQL Cache

Let's make a table to store users data:

@snippet samples/postgres_auth/schemas/postgresql/auth.sql  postgresql schema

Authorization data is rarely changed and often queried. Caching it would improve
response times:

@snippet samples/postgres_auth/user_info_cache.hpp  user info cache

Cache configuration is straightforward:

@snippet samples/postgres_auth/static_config.yaml  pg-cache config


### Authorization Checker

To implement an authorization checker derive from 
server::handlers::auth::AuthCheckerBase and override the virtual functions:

@snippet samples/postgres_auth/auth_bearer.cpp  auth checker declaration


The authorization should do the following steps:
* check for "Authorization" header, return 401 if it is missing;
  @snippet samples/postgres_auth/auth_bearer.cpp  auth checker definition 1
* check for "Authorization" header value to have `Bearer some-token` format,
  return 403 if it is not;
  @snippet samples/postgres_auth/auth_bearer.cpp  auth checker definition 2
* check that the token is in the cache, return 403 if it is not;
  @snippet samples/postgres_auth/auth_bearer.cpp  auth checker definition 3
* check that user has the required authorization scopes, return 403 if not;
  @snippet samples/postgres_auth/auth_bearer.cpp  auth checker definition 4
* if everything is fine, then store user name in request context and proceed to
  HTTP handler logic.
  @snippet samples/postgres_auth/auth_bearer.cpp  auth checker definition 5


@warning `CheckAuth` functions are invoked concurrently on the same instance of
  the class. In this sample the `AuthCheckerBearer` class only reads the
  class data. @ref md_en_userver_synchronization "synchronization primitives"
  should be used if data is mutated.


### Authorization Factory

Authorization checkers are produced by authorization factories derived from
server::handlers::auth::AuthCheckerFactoryBase:

@snippet samples/postgres_auth/auth_bearer.hpp  auth checker factory decl


Factories work with component system and parse handler-specific
authorization configs:

@snippet samples/postgres_auth/auth_bearer.cpp  auth checker factory definition


Each factory should be registered at the beginning of the `main()` function via
server::handlers::auth::RegisterAuthCheckerFactory function call:

@snippet samples/postgres_auth/postgres_service.cpp  auth checker registration


That factory is invoked on each HTTP handler with the matching authorization
type:

@snippet samples/postgres_auth/static_config.yaml  hello config


### Request Context

Data that was set by authorization checker could be retrieved by handler from
server::request::RequestContext:

@snippet samples/postgres_auth/postgres_service.cpp  request context


### int main()

Aside from calling server::handlers::auth::RegisterAuthCheckerFactory for
authorization factory registration, the `main()` function should also
construct the component list and start the daemon:

@snippet samples/postgres_auth/postgres_service.cpp  main


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-postgres_auth
```

The sample could be started by running
`make start-userver-samples-postgres_auth`. The command would invoke
@ref md_en_userver_functional_testing "testsuite start target" that sets proper
paths in the configuration files, prepares and starts the DB, and starts the
service.

To start the service manually start the DB server and run
`./samples/postgres_service/userver-samples-postgres_auth -c </path/to/static_config.yaml>`
(do not forget to prepare the configuration files!).

Now you can send a request to your service from another terminal:
```
bash
$ curl 'localhost:8080/v1/hello' -i
HTTP/1.1 401 Unauthorized
Date: Wed, 21 Dec 2022 13:04:17 UTC
Content-Type: text/html
X-YaRequestId: dbc9dbaa3fc04ce8a86b27a1aa582cd6
X-YaSpanId: aa573144f2312714
X-YaTraceId: 4dfb9e852e07473c9d57a8eb520e7965
Server: userver/1.0.0 (20221221124812; rv:unknown)
Connection: keep-alive
Content-Length: 28

Empty 'Authorization' header

$ curl -H 'Authorization: Bearer SOME_WRONG_USER_TOKEN' 'localhost:8080/v1/hello' -i
HTTP/1.1 403 Forbidden
Date: Wed, 21 Dec 2022 13:06:06 UTC
Content-Type: text/html
X-YaRequestId: 6e39f3bf27324aa3acb01a30b9653b2d
X-YaTraceId: e5d38ab53b3f495a9b97279a731f5fde
X-YaSpanId: e64b939c37035d88
Server: userver/1.0.0 (20221221124812; rv:unknown)
Connection: keep-alive
Content-Length: 0
```


### Functional testing
@ref md_en_userver_functional_testing "Functional tests" for the service could be
implemented using the testsuite. To do that you have to:

* Provide PostgreSQL schema to start the database:
  @snippet samples/postgres_auth/schemas/postgresql/auth.sql  postgresql schema
* Tell the testsuite to start the PostgreSQL database by adjusting the
  @ref samples/postgres_auth/tests/conftest.py
* Prepare the DB test data @ref samples/postgres_auth/tests/static/test_data.sql
* Write the test:
  @snippet samples/postgres_auth/tests/test_postgres.py  Functional test


## Full sources

See the full example:
* @ref samples/postgres_auth/user_info_cache.hpp
* @ref samples/postgres_auth/auth_bearer.hpp
* @ref samples/postgres_auth/auth_bearer.cpp
* @ref samples/postgres_auth/postgres_service.cpp
* @ref samples/postgres_auth/static_config.yaml
* @ref samples/postgres_auth/dynamic_config_fallback.json
* @ref samples/postgres_auth/CMakeLists.txt
* @ref samples/postgres_auth/schemas/postgresql/auth.sql
* @ref samples/postgres_auth/tests/conftest.py
* @ref samples/postgres_auth/tests/test_postgres.py
* @ref samples/postgres_auth/tests/static/test_data.sql

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_tutorial_redis_service | @ref md_en_userver_component_system ⇨
@htmlonly </div> @endhtmlonly

@example samples/postgres_auth/user_info_cache.hpp
@example samples/postgres_auth/auth_bearer.hpp
@example samples/postgres_auth/auth_bearer.cpp
@example samples/postgres_auth/postgres_service.cpp
@example samples/postgres_auth/static_config.yaml
@example samples/postgres_auth/dynamic_config_fallback.json
@example samples/postgres_auth/CMakeLists.txt
@example samples/postgres_auth/schemas/postgresql/auth.sql
@example samples/postgres_auth/tests/conftest.py
@example samples/postgres_auth/tests/test_postgres.py
@example samples/postgres_auth/tests/static/test_data.sql
