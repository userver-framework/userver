#pragma once

/// @file userver/ydb/fwd.hpp
/// @brief Forward declarations of YDB clients and component

USERVER_NAMESPACE_BEGIN

namespace ydb {

class TableClient;
class TopicClient;
class FederatedTopicClient;
class CoordinationClient;
class YdbComponent;

}  // namespace ydb

USERVER_NAMESPACE_END
