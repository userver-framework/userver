#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace tracing {

// The following tags are described in greater detail at the following url:
// https://github.com/opentracing/specification/blob/master/semantic_conventions.md
//
// Here we define standard names for tags that can be added to spans by the
// instrumentation code. The actual tracing systems are not required to
// retain these as tags in the stored spans if they have other means of
// representing the same data. For example, the SPAN_KIND='server' can be
// inferred from a Zipkin span by the presence of ss/sr annotations.

extern const std::string kType;
extern const std::string kHttpUrl;
extern const std::string kHttpMetaType;
extern const std::string kHttpMethod;
extern const std::string kHttpStatusCode;
extern const std::string kAttempts;
extern const std::string kMaxAttempts;
extern const std::string kTimeoutMs;
extern const std::string kErrorFlag;
extern const std::string kErrorMessage;

extern const std::string kDatabaseType;
extern const std::string kDatabaseMongoType;
extern const std::string kDatabasePostgresType;
extern const std::string kDatabaseRedisType;

extern const std::string kDatabaseCollection;
extern const std::string kDatabaseInstance;
extern const std::string kDatabaseStatement;
extern const std::string kDatabaseStatementName;
extern const std::string kDatabaseStatementDescription;

extern const std::string kPeerAddress;

}  // namespace tracing

USERVER_NAMESPACE_END
