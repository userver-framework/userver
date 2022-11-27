#include "userver/storages/etcd/watch.hpp"

#include <chrono>
#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>
#include <memory>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/engine/sleep.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

WatchClient::WatchClient(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context)
      : userver::components::LoggableComponentBase(config, context),
        grpc_client_factory_(
            context.FindComponent<userver::ugrpc::client::ClientFactoryComponent>()
                .GetFactory()),
        create_cooldown(config["create_cooldown"].As<int64_t>()),
        to_stop(false),
        to_reset(false),
        watch_callback_set(false),
        watch_start(true) {
              const auto endpoints = config["endpoints"].As<std::vector<std::string>>();
              grpc_watch_client_ = std::make_shared<etcdserverpb::WatchClient>(grpc_client_factory_.MakeClient<etcdserverpb::WatchClient>(endpoints.front()));

              bts.AsyncDetach("task_watch", [&](){
                while(!userver::engine::current_task::ShouldCancel() && !to_stop){
                  Reset();
                }
              });
            }

userver::ugrpc::client::BidirectionalStream<etcdserverpb::WatchRequest, etcdserverpb::WatchResponse> WatchClient::Init(){
  ::etcdserverpb::WatchRequest request;
  etcdserverpb::WatchCreateRequest create_request;
  create_request.set_key(/*key*/ ".");
  create_request.set_range_end(/*range_end*/std::string(1,'.'+1));
  create_request.set_start_revision(1);
  *request.mutable_create_request() = create_request;

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_fail_fast(true);
  auto stream = grpc_watch_client_->Watch(std::move(context));
  etcdserverpb::WatchResponse response; 

  try{
    stream.Write(request);
    if(!stream.Read(response)){
      to_reset = true;
      return stream;
    }
  } catch(userver::ugrpc::client::RpcError& er){
    LOG_CRITICAL() << "Read error";
    to_reset = true;
    return stream;
  }

  if(!response.created()){
    to_reset = true;
    return stream;
  }

  watch_id = response.watch_id();
  to_reset = false;
  watch_start = true;
  return stream;
}

void WatchClient::Reset(/*std::string& key, std::string& range_end*/){
  etcdserverpb::WatchResponse response; 

  auto stream = Init();
  if(to_reset){
    userver::engine::SleepFor(std::chrono::milliseconds{create_cooldown}); 
    return;
  }

  while(!userver::engine::current_task::ShouldCancel()){
    if(!watch_callback_set){
      userver::engine::SleepFor(std::chrono::milliseconds{50}); 
      continue;
    }

    if(to_reset){
      LOG_DEBUG() << "Reseting stream";
      Destroy(stream);
      stream = Init();
    }

    try{
      if(!stream.Read(response)){
        break;
      }
    } catch(userver::ugrpc::client::RpcError& er){
      LOG_DEBUG() << "Read error";
      to_reset = true;
      break;
    }

    if(to_stop){
      return;
    }

    if(watch_start){
      for(int i = 0; i < response.mutable_events()->size(); ++i){
        while((i + 1 < response.mutable_events()->size()) && (((*response.mutable_events())[i]).kv().key() == ((*response.mutable_events())[i + 1]).kv().key())){
          ++i;
        }

        if(i + 1 == response.mutable_events()->size()){
          
        }

        auto& cur = (*response.mutable_events())[i];

        if(cur.type() == mvccpb::Event_EventType::Event_EventType_PUT){
          LOG_DEBUG() << "reported event: PUT " + cur.kv().key() + " " + cur.kv().value();
        } else{
          LOG_DEBUG() << "reported event: DELETE " + cur.kv().key() + " " + cur.kv().value();
        }

        //watch_callback(cur.type() == mvccpb::Event_EventType::Event_EventType_PUT, cur.kv().key(), cur.kv().value());
      }
      watch_start = false;
    } else{
      for(auto& i : *response.mutable_events()){   
        //watch_callback(i.type() == mvccpb::Event_EventType::Event_EventType_PUT, i.kv().key(), i.kv().value());
        if(i.type() == mvccpb::Event_EventType::Event_EventType_PUT){
          LOG_DEBUG() << "reported event: PUT " + i.kv().key() + " " + i.kv().value();
        } else{
          LOG_DEBUG() << "reported event: DELETE " + i.kv().key() + " " + i.kv().value();
        }
        //LOG_CRITICAL() << "reported event: PUT " + i.kv().key() + " " + i.kv().value();
      }
    }
  }
  Destroy(stream);
  userver::engine::SleepFor(std::chrono::milliseconds{create_cooldown});
}

void WatchClient::Destroy(userver::ugrpc::client::BidirectionalStream<::etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>& stream){
  if(to_reset){
    return;
  }
  
  etcdserverpb::WatchRequest second_request;
  etcdserverpb::WatchCancelRequest cancel_request;
  cancel_request.set_watch_id(watch_id);
  second_request.set_allocated_cancel_request(&cancel_request);
  stream.Write(second_request);
  stream.WritesDone();

  etcdserverpb::WatchResponse response; 
  try{
    if(!stream.Read(response)){
      return;
    }
  } catch(userver::ugrpc::client::RpcError& er){
    LOG_DEBUG() << "Read error";
    to_reset = true;
    return;
  }
  if(!response.canceled()){
    return;
  }

  stream.Finish();
}

void WatchClient::SetCallback(std::function<void(bool, std::string, std::string)> func) {
  watch_callback = std::move(func);
  watch_callback_set = true;
}

bool WatchClient::IsCallbackSet() const{
  return watch_callback_set;
}

WatchClient::~WatchClient() {
  bts.CancelAndWait();
}

} // namespace storages::etcd

USERVER_NAMESPACE_END
