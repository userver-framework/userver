#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

const std::string kType = "_type";

const std::string kHttpUrl = "http.url";

const std::string kHttpMetaType = "meta_type";

const std::string kHttpMethod = "method";

const std::string kHttpStatusCode = "meta_code";

const std::string kAttempts = "attempts";

const std::string kMaxAttempts = "max_attempts";

const std::string kTimeoutMs = "timeout_ms";

const std::string kErrorFlag = "error";

const std::string kErrorMessage = "error_msg";

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

}  // namespace tracing

USERVER_NAMESPACE_END
