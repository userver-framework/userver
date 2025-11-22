#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

const std::string kType = "_type";

const std::string kSpanKind = "span_kind";

const std::string kSpanKindServer = "server";

const std::string kSpanKindClient = "client";

const std::string kSpanKindInternal = "internal";

const std::string kHttpUrl = "http.url";

const std::string kUrlFull = "url.full";

const std::string kHttpUrlTemplate = "url.template";

const std::string kHttpMetaType = "meta_type";

const std::string kHttpMethod = "method";

const std::string kHttpRequestMethod = "http.request.method";

const std::string kHttpStatusCode = "meta_code";

const std::string kHttpResponseStatusCode = "meta_code";

const std::string kHttpRoute = "http.route";

const std::string kAttempts = "http.request.resend_count";

const std::string kMaxAttempts = "http.request.max_resend_count";

const std::string kTimeoutMs = "timeout_ms";

const std::string kErrorFlag = "error";

const std::string kErrorMessage = "error_msg";

const std::string kRpcSystem = "rpc.system";

const std::string kRpcService = "rpc.service";

const std::string kRpcMethod = "rpc.method";

const std::string kGrpcCode = "rpc.grpc.status_code";

const std::string kDatabaseType = "db.type";

const std::string kDatabaseMongoType = "mongo";

const std::string kDatabasePostgresType = "postgres";

const std::string kDatabaseRedisType = "redis";

const std::string kDatabaseCollection = "db.collection";

const std::string kDatabaseInstance = "db.instance";

const std::string kDatabaseStatement = "db.statement";

const std::string kDatabaseStatementName = "db.statement_name";

const std::string kDatabaseStatementDescription = "db.statement_description";

const std::string kPeerAddress = "peer.address";

const std::string kServerAddress = "server.address";

const std::string kUserAgent = "useragent";

}  // namespace tracing

USERVER_NAMESPACE_END
