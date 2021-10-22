#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::scopes {

// The following tags are used to describe stages in Spans that trace PostgreSQL
// driver. pg_* stages can include libpq_* stages
/// Connect stage, driver level
const std::string kConnect = "pg_connect";
/// Running misc queries after connecting
const std::string kGetConnectData = "pg_get_conn_data";
/// Execute query, top driver level
const std::string kQuery = "pg_query";
/// Prepare query, driver level
const std::string kPrepare = "pg_prepare";
/// Bind portal, driver level
const std::string kBind = "pg_bind";
/// Execute query, driver level
const std::string kExec = "pg_exec";

// libpq stages
/// libpq async connect stage
const std::string kLibpqConnect = "libpq_async_connect";
/// libpq wait connection stage
const std::string kLibpqWaitConnectFinish = "libpq_wait_connect_finish";
/// libpq wait result stage
const std::string kLibpqWaitResult = "libpq_wait_result";
/// libpq send query params stage
const std::string kLibpqSendQueryParams = "libpq_send_query_params";
/// libpq send prepare stage
const std::string kLibpqSendPrepare = "libpq_send_prepare";
/// libpq send describe prepared stage
const std::string kLibpqSendDescribePrepared = "libpq_send_describe_prepared";
/// libpq send query prepared stage
const std::string kLibpqSendQueryPrepared = "libpq_send_query_prepared";
/// libpq-missing send bind portal
const std::string kPqSendPortalBind = "pq_send_portal_bind";
/// libpq-missing send execute portal
const std::string kPqSendPortalExecute = "pq_send_portal_execute";

}  // namespace storages::postgres::scopes

USERVER_NAMESPACE_END
