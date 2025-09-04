 ## YDB

**Quality:** @ref QUALITY_TIERS "Platinum Tier".

YDB asynchronous driver provides interface to work with Tables, Topics and Coordination Service.

* Table (userver/ydb/table.hpp);
* Topic (userver/ydb/transaction.hpp);
* Coordination Service (userver/ydb/coordination.hpp);

## Usage

To use YDB you have to add the component ydb::YdbComponent and configure it according to the documentation.
From component you can access required client (ydb::TableClient, ydb::TopicClient, ydb::CoordinationClient).
You may store YQL queries in @ref scripts/docs/en/userver/sql_files.md.

## Examples

[Sample YDB usage](https://github.com/userver-framework/userver/tree/develop/samples/ydb_service)
[YDB tests](https://github.com/userver-framework/userver/tree/develop/ydb/tests)

## More information
- https://ydb.tech/

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/kafka.md |
@ref scripts/docs/en/userver/sqlite/sqlite_driver.md ⇨
@htmlonly </div> @endhtmlonly
