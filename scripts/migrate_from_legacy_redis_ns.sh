#!/bin/bash

# Exit on any error and treat unset variables as errors, print all commands
set -euox pipefail

USERVER_NAMESPACE="${NAMESPACE:-userver}"

find . -name '*pp' -type f | xargs sed -i \
    -e 's|userver/storages/redis/impl/reply.hpp|userver/storages/redis/reply.hpp|g' \
    -e 's|userver/storages/redis/impl/base.hpp|userver/storages/redis/base.hpp|g' \
    -e 's|userver/storages/redis/impl/request.hpp|userver/storages/redis/request.hpp|g' \
    -e 's|userver/storages/redis/impl/request.hpp|userver/storages/redis/request.hpp|g' \
    -e 's|userver/storages/redis/impl/request.hpp|userver/storages/redis/request.hpp|g' \
    \
    -e "s| $USERVER_NAMESPACE::redis::CmdArgs| $USERVER_NAMESPACE::storages::redis::CmdArgs|g" \
    -e "s| $USERVER_NAMESPACE::redis::CommandsBufferingSettings| $USERVER_NAMESPACE::storages::redis::CommandsBufferingSettings|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ConnectionInfo| $USERVER_NAMESPACE::storages::redis::ConnectionInfo|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ConnectionSecurity| $USERVER_NAMESPACE::storages::redis::ConnectionSecurity|g"  \
    -e "s| $USERVER_NAMESPACE::redis::Password| $USERVER_NAMESPACE::storages::redis::Password|g"  \
    -e "s| $USERVER_NAMESPACE::redis::PublishSettings| $USERVER_NAMESPACE::storages::redis::PublishSettings|g"  \
    -e "s| $USERVER_NAMESPACE::redis::PubsubMetricsSettings| $USERVER_NAMESPACE::storages::redis::PubsubMetricsSettings|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ReplicationMonitoringSettings| $USERVER_NAMESPACE::storages::redis::ReplicationMonitoringSettings|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ScanCursor| $USERVER_NAMESPACE::storages::redis::ScanCursor|g"  \
    -e "s| $USERVER_NAMESPACE::redis::Stat| $USERVER_NAMESPACE::storages::redis::Stat|g"  \
    \
    -e "s| $USERVER_NAMESPACE::redis::kDefaultMaxRetries| $USERVER_NAMESPACE::storages::redis::kDefaultMaxRetries|g"  \
    -e "s| $USERVER_NAMESPACE::redis::kDefaultTimeoutAll| $USERVER_NAMESPACE::storages::redis::kDefaultTimeoutAll|g"  \
    -e "s| $USERVER_NAMESPACE::redis::kDefaultTimeoutSingle| $USERVER_NAMESPACE::storages::redis::kDefaultTimeoutSingle|g"  \
    \
    -e "s| $USERVER_NAMESPACE::redis::kRetryNilFromMaster| $USERVER_NAMESPACE::storages::redis::kRetryNilFromMaster|g"  \
    -e "s| $USERVER_NAMESPACE::redis::RetryNilFromMaster| $USERVER_NAMESPACE::storages::redis::RetryNilFromMaster|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ServerIdHasher| $USERVER_NAMESPACE::storages::redis::ServerIdHasher|g"  \
    \
    -e "s| $USERVER_NAMESPACE::redis::CommandControl| $USERVER_NAMESPACE::storages::redis::CommandControl|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ServerId| $USERVER_NAMESPACE::storages::redis::ServerId|g"  \
    -e "s| $USERVER_NAMESPACE::redis::StrategyFromString| $USERVER_NAMESPACE::storages::redis::StrategyFromString|g"  \
    -e "s| $USERVER_NAMESPACE::redis::StrategyToString| $USERVER_NAMESPACE::storages::redis::StrategyToString|g"  \
    \
    -e "s| $USERVER_NAMESPACE::redis::ClientNotConnectedException| $USERVER_NAMESPACE::storages::redis::ClientNotConnectedException|g"  \
    -e "s| $USERVER_NAMESPACE::redis::Exception| $USERVER_NAMESPACE::storages::redis::Exception|g"  \
    -e "s| $USERVER_NAMESPACE::redis::InvalidArgumentException| $USERVER_NAMESPACE::storages::redis::InvalidArgumentException|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ParseConfigException| $USERVER_NAMESPACE::storages::redis::ParseConfigException|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ParseReplyException| $USERVER_NAMESPACE::storages::redis::ParseReplyException|g"  \
    -e "s| $USERVER_NAMESPACE::redis::RequestCancelledException| $USERVER_NAMESPACE::storages::redis::RequestCancelledException|g"  \
    -e "s| $USERVER_NAMESPACE::redis::RequestFailedException| $USERVER_NAMESPACE::storages::redis::RequestFailedException|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ExpireReply| $USERVER_NAMESPACE::storages::redis::ExpireReply|g"  \
    -e "s| $USERVER_NAMESPACE::redis::HedgedRedisRequest| $USERVER_NAMESPACE::storages::redis::HedgedRedisRequest|g"  \
    -e "s| $USERVER_NAMESPACE::redis::MakeBulkHedgedRedisRequest| $USERVER_NAMESPACE::storages::redis::MakeBulkHedgedRedisRequest|g"  \
    -e "s| $USERVER_NAMESPACE::redis::MakeBulkHedgedRedisRequestAsync| $USERVER_NAMESPACE::storages::redis::MakeBulkHedgedRedisRequestAsync|g"  \
    -e "s| $USERVER_NAMESPACE::redis::MakeHedgedRedisRequest| $USERVER_NAMESPACE::storages::redis::MakeHedgedRedisRequest|g"  \
    -e "s| $USERVER_NAMESPACE::redis::MakeHedgedRedisRequestAsync| $USERVER_NAMESPACE::storages::redis::MakeHedgedRedisRequestAsync|g"  \
    \
    -e "s| $USERVER_NAMESPACE::redis::Reply| $USERVER_NAMESPACE::storages::redis::Reply|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ReplyData| $USERVER_NAMESPACE::storages::redis::ReplyData|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ReplyPtr| $USERVER_NAMESPACE::storages::redis::ReplyPtr|g"  \
    -e "s| $USERVER_NAMESPACE::redis::ReplyStatus| $USERVER_NAMESPACE::storages::redis::ReplyStatus|g"  \
    \
    -e "s| $USERVER_NAMESPACE::redis::KeyHasNoExpirationException| $USERVER_NAMESPACE::storages::redis::KeyHasNoExpirationException|g"  \
    -e "s| $USERVER_NAMESPACE::redis::TtlReply| $USERVER_NAMESPACE::storages::redis::TtlReply|g"  \
    \
    -e "s| $USERVER_NAMESPACE::redis::kRedisWaitConnectedDefaultTimeout| $USERVER_NAMESPACE::storages::redis::kRedisWaitConnectedDefaultTimeout|g"  \
    -e "s| $USERVER_NAMESPACE::redis::RedisWaitConnected| $USERVER_NAMESPACE::storages::redis::RedisWaitConnected|g"  \
    -e "s| $USERVER_NAMESPACE::redis::WaitConnectedMode| $USERVER_NAMESPACE::storages::redis::WaitConnectedMode|g"  \
    \
    \
    -e "s| redis::CmdArgs| storages::redis::CmdArgs|g" \
    -e "s| redis::CommandsBufferingSettings| storages::redis::CommandsBufferingSettings|g"  \
    -e "s| redis::ConnectionInfo| storages::redis::ConnectionInfo|g"  \
    -e "s| redis::ConnectionSecurity| storages::redis::ConnectionSecurity|g"  \
    -e "s| redis::Password| storages::redis::Password|g"  \
    -e "s| redis::PublishSettings| storages::redis::PublishSettings|g"  \
    -e "s| redis::PubsubMetricsSettings| storages::redis::PubsubMetricsSettings|g"  \
    -e "s| redis::ReplicationMonitoringSettings| storages::redis::ReplicationMonitoringSettings|g"  \
    -e "s| redis::ScanCursor| storages::redis::ScanCursor|g"  \
    -e "s| redis::Stat| storages::redis::Stat|g"  \
    \
    -e "s| redis::kDefaultMaxRetries| storages::redis::kDefaultMaxRetries|g"  \
    -e "s| redis::kDefaultTimeoutAll| storages::redis::kDefaultTimeoutAll|g"  \
    -e "s| redis::kDefaultTimeoutSingle| storages::redis::kDefaultTimeoutSingle|g"  \
    \
    -e "s| redis::kRetryNilFromMaster| storages::redis::kRetryNilFromMaster|g"  \
    -e "s| redis::RetryNilFromMaster| storages::redis::RetryNilFromMaster|g"  \
    -e "s| redis::ServerIdHasher| storages::redis::ServerIdHasher|g"  \
    \
    -e "s| redis::CommandControl| storages::redis::CommandControl|g"  \
    -e "s| redis::ServerId| storages::redis::ServerId|g"  \
    -e "s| redis::StrategyFromString| storages::redis::StrategyFromString|g"  \
    -e "s| redis::StrategyToString| storages::redis::StrategyToString|g"  \
    \
    -e "s| redis::ClientNotConnectedException| storages::redis::ClientNotConnectedException|g"  \
    -e "s| redis::Exception| storages::redis::Exception|g"  \
    -e "s| redis::InvalidArgumentException| storages::redis::InvalidArgumentException|g"  \
    -e "s| redis::ParseConfigException| storages::redis::ParseConfigException|g"  \
    -e "s| redis::ParseReplyException| storages::redis::ParseReplyException|g"  \
    -e "s| redis::RequestCancelledException| storages::redis::RequestCancelledException|g"  \
    -e "s| redis::RequestFailedException| storages::redis::RequestFailedException|g"  \
    -e "s| redis::ExpireReply| storages::redis::ExpireReply|g"  \
    -e "s| redis::HedgedRedisRequest| storages::redis::HedgedRedisRequest|g"  \
    -e "s| redis::MakeBulkHedgedRedisRequest| storages::redis::MakeBulkHedgedRedisRequest|g"  \
    -e "s| redis::MakeBulkHedgedRedisRequestAsync| storages::redis::MakeBulkHedgedRedisRequestAsync|g"  \
    -e "s| redis::MakeHedgedRedisRequest| storages::redis::MakeHedgedRedisRequest|g"  \
    -e "s| redis::MakeHedgedRedisRequestAsync| storages::redis::MakeHedgedRedisRequestAsync|g"  \
    \
    -e "s| redis::Reply| storages::redis::Reply|g"  \
    -e "s| redis::ReplyData| storages::redis::ReplyData|g"  \
    -e "s| redis::ReplyPtr| storages::redis::ReplyPtr|g"  \
    -e "s| redis::ReplyStatus| storages::redis::ReplyStatus|g"  \
    \
    -e "s| redis::KeyHasNoExpirationException| storages::redis::KeyHasNoExpirationException|g"  \
    -e "s| redis::TtlReply| storages::redis::TtlReply|g"  \
    \
    -e "s| redis::kRedisWaitConnectedDefaultTimeout| storages::redis::kRedisWaitConnectedDefaultTimeout|g"  \
    -e "s| redis::RedisWaitConnected| storages::redis::RedisWaitConnected|g"  \
    -e "s| redis::WaitConnectedMode| storages::redis::WaitConnectedMode|g"
