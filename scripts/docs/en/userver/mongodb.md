## MongoDB

The mongo asynchronous driver provides an interface to work with MongoDB
databases and the BSON format.

## Main features
* Building and reading BSON documents with support for most of the C++ types;
* Support for basic operations with collections via storages::mongo::Collection;
* Support for bulk operations;
* Dynamic management of database sets;
* Aggregation support.

## Metrics

Most important ones:

| Metric name                     | Description                                          |
|---------------------------------|------------------------------------------------------|
| mongo.pool.current-size         | current number of open connections                   |
| mongo.pool.max-size             | limit on the number of open connections              |
| mongo.pool.overloads            | counter of requests that could not get a connection  |
| mongo.pool.queue-wait-timings   | waiting timings in the queue to receive a connection |
| mongo.pool.conn-request-timings | connection receipt timings (includes queue-wait)     |
| mongo.pool.conn-created/closed  | open/closed connection counters                      |
| mongo.success                   | counter of successfully executed requests            |
| mongo.errors                    | counter of failed requests                           |
| mongo.timings                   | query timings                                        |

See @ref md_en_userver_service_monitor for info on how to get the metrics.


## Usage

To use MongoDB you have to add the component components::Mongo and configure it
according to the documentation. After that you can work with a collection via
storages::mongo::Collection:

@snippet storages/mongo/collection_mongotest.hpp  Sample Mongo usage

Methods and options correspond to [the standard MongoDB Collections API](https://docs.mongodb.com/manual/reference/method/#collection).


### Timeouts

It is recommended to set timeouts on the server side using
storages::mongo::options::MaxServerTime as for other cancellation mechanisms
MongoDB does not guarantee that the cancelled operation was not applied.

For client-side timeouts consider setting socket and/or connection timeouts
in pool configuration as native task cancellation is not supported in the
current implementation.


### Write results

Methods that modify a collection return an object storages::mongo::WriteResult
containing information about the result of the operation:

@snippet storages/mongo/collection_mongotest.hpp  Sample Mongo write result


### Packaging operations

In cases where the same operation is performed with the same arguments many
times, it can be stored into a variable and reused. The finished operation
can be passed to the collection method `Execute()`.

@snippet storages/mongo/collection_mongotest.hpp  Sample Mongo packaged operation


### BSON

BSON follows the common formats interface that is described in detail at
@ref md_en_userver_formats.

Since BSON is a binary format, it is not human readable. Differences
in the BSON and JSON formats type systems do not always allow you to perform
conversions between them unambiguously. In this regard, we do not provide
conversion functions between BSON and text formats by default, but they are
available in <userver/formats/bson/serialize.hpp>. These functions are provided
without guarantees for the stability of the conversion, and are primarily
intended for debugging.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref pg_bytea | @ref md_en_userver_redis ⇨
@htmlonly </div> @endhtmlonly
