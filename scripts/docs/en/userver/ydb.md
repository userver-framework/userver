 ## YDB

**Quality:** @ref QUALITY_TIERS "Platinum Tier".

YDB asynchronous driver provides interface to work with Tables, Topics and Coordination Service.


## Usage

To use YDB you have to add the component @ref ydb::YdbComponent and configure it according to the documentation.
From component you can access required client:

* @ref ydb::TableClient
* @ref ydb::TopicClient
* @ref ydb::CoordinationClient

For writing messages to YDB topics, userver also provides @ref ydb::TopicWriter. It can be constructed directly or
obtained from @ref ydb::TopicWriterManager via @ref ydb::TopicWriterComponent.

You may store YQL queries in @ref scripts/docs/en/userver/sql_files.md.


## Examples

* @ref scripts/docs/en/userver/tutorial/ydb_service.md
* @ref scripts/docs/en/userver/tutorial/ydb_topic_writer_service.md


## More information
- https://ydb.tech/

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/kafka.md | @ref scripts/docs/en/userver/sqlite/sqlite_driver.md ⇨
@htmlonly </div> @endhtmlonly
