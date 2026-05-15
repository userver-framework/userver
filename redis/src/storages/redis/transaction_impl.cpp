#include <storages/redis/transaction_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

TransactionImpl::TransactionImpl(std::shared_ptr<ClientImpl> client, CheckShards check_shards)
    : PipelineImpl(std::move(client), check_shards) {
    CmdArgs().Then("MULTI");
}

RequestExec TransactionImpl::Exec(const CommandControl& command_control) {
    CmdArgs().Then("EXEC");

    return PipelineImpl::Exec(command_control);
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
