#include <tracing/tags.hpp>

namespace tracing {

const std::string kType = "_type";

const std::string kHttpUrl = "http.url";

const std::string kHttpMetaType = "meta_type";

const std::string kHttpMethod = "method";

const std::string kHttpStatusCode = "meta_code";

const std::string kAttempts = "attempts";

const std::string kErrorFlag = "error";

const std::string kErrorMessage = "error_msg";

const std::string kDatabaseType = "db.type";

const std::string kDatabaseMongoType = "mongo";

const std::string kDatabasePostgresType = "postgres";

const std::string kDatabaseCollection = "db.collection";

const std::string kDatabaseInstance = "db.instance";

const std::string kDatabaseStatement = "db.statement";

}  // namespace tracing
