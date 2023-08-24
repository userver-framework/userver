## Digest Authorization/Authentication via PostgreSQL

## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref scripts/docs/en/userver/tutorial/hello_service.md.


## Step by step guide

This tutorial shows how to create a custom digest authorization checker. In
the tutorial the authorization data is stored in PostgreSQL database, and
information of an authorized user (i.e. Authorization header) 
is passed to the HTTP handler.

Authentication credentials checking logic is set in base class `DigestCheckerBase`, 
This sample simply defines derived class `AuthCheckerDigest`, which gets, sets user data, and pushes unnamed nonce in pool.
Digest authentication logic and hashing logic is out of scope of this tutorial.
For reference, read [https://datatracker.ietf.org/doc/html/rfc2617](RFC2617).


### PostgreSQL Table

Let's make a table to store users data:

@snippet samples/digest_auth_service/postgresql/schemas/auth.sql  postgresql schema


### Authorization Checker

To implement an authorization checker derive from 
`server::handlers::auth::AuthDigestCheckerBase` and override the virtual functions:

@snippet samples/digest_auth_service/auth_digest.cpp  auth checker declaration


The authorization in base class calls the following functions:
* Returns user data from database;
  @snippet samples/digest_auth_service/auth_digest.cpp  auth checker definition 1
* Inserts user data into database;
  @snippet samples/digest_auth_service/auth_digest.cpp  auth checker definition 2
* Pushes unnamed nonce into database using kInsertUnnameNonce query
  @snippet samples/digest_auth_service/auth_digest.cpp  auth checker definition 3
* kInsertUnnameNonce query looks like this
  @snippet samples/digest_auth_service/sql/queries.hpp  insert unnamed nonce


### Authorization Factory

Authorization checkers are produced by authorization factories derived from
server::handlers::auth::AuthCheckerFactoryBase:

@snippet samples/digest_auth_service/auth_digest.hpp  auth checker factory decl


Factories work with component system and parse handler-specific
authorization configs:

@snippet samples/digest_auth_service/auth_digest.cpp  auth checker factory definition


Each factory should be registered at the beginning of the `main()` function via
server::handlers::auth::RegisterAuthCheckerFactory function call:

@snippet samples/digest_auth_service/postgres_service.cpp  auth checker registration


That factory is invoked on each HTTP handler with the matching authorization
type:

@snippet samples/digest_auth_service/static_config.yaml hello config

Digest settings are universal for each handler, which uses digest authentication

@snippet samples/digest_auth_service/static_config.yaml digest config


### Request Context

Data that was set by authorization checker could be retrieved by handler from
server::request::RequestContext:

@snippet samples/digest_auth_service/postgres_service.cpp  request context


### int main()

Aside from calling server::handlers::auth::RegisterAuthCheckerFactory for
authorization factory registration, the `main()` function should also
construct the component list and start the daemon:

@snippet samples/digest_auth_service/postgres_service.cpp  main


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-userver-samples-digest_auth
```

The sample could be started by running
`make start-userver-samples-userver-samples-digest_auth`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files, prepares and starts the DB, and starts the
service.

To start the service manually start the DB server and run
`./samples/postgres_service/userver-samples-userver-samples-digest_auth -c </path/to/static_config.yaml>`
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
@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the testsuite. To do that you have to:

* Provide PostgreSQL schema to start the database:
  @snippet samples/digest_auth_service/postgresql/schemas/auth.sql  postgresql schema
* Tell the testsuite to start the PostgreSQL database by adjusting the
  @ref samples/digest_auth_service/tests/conftest.py
* Prepare the DB test data @ref samples/digest_auth_service/tests/static/test_data.sql
* Write the test:
  @snippet samples/digest_auth_service/tests/test_postgres.py  Functional test


## Full sources

See the full example:
* @ref samples/digest_auth_service/auth_digest.hpp
* @ref samples/digest_auth_service/auth_digest.cpp
* @ref samples/digest_auth_service/static_config.yaml
* @ref samples/digest_auth_service/dynamic_config_fallback.json
* @ref samples/digest_auth_service/CMakeLists.txt
* @ref samples/digest_auth_service/schemas/postgresql/auth.sql
* @ref samples/digest_auth_service/tests/conftest.py
* @ref samples/digest_auth_service/tests/test_postgres.py
* @ref samples/digest_auth_service/tests/static/test_data.sql

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/redis_service.md | @ref scripts/docs/en/userver/tutorial/json_to_yaml.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/digest_auth_service/auth_digest.hpp
@example samples/digest_auth_service/auth_digest.cpp
@example samples/digest_auth_service/static_config.yaml
@example samples/digest_auth_service/dynamic_config_fallback.json
@example samples/digest_auth_service/CMakeLists.txt
@example samples/digest_auth_service/schemas/postgresql/auth.sql
@example samples/digest_auth_service/tests/conftest.py
@example samples/digest_auth_service/tests/test_postgres.py
@example samples/digest_auth_service/tests/static/test_data.sql

