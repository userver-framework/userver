#pragma once

#include <storages/redis/pipeline_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class TransactionImpl final : public PipelineImpl {
public:
    explicit TransactionImpl(std::shared_ptr<ClientImpl> client, CheckShards check_shards = CheckShards::kSame);

    RequestExec Exec(const CommandControl& command_control) override;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
