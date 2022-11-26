#include "userver/storages/etcd/watch.hpp"

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace storages::etcd {

WatchClient::WatchClient(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context)
      : userver::components::LoggableComponentBase(config, context),
        grpc_client_factory_(
            context.FindComponent<userver::ugrpc::client::ClientFactoryComponent>()
                .GetFactory()),
        grpc_watch_client_(grpc_client_factory_.MakeClient<etcdserverpb::WatchClient>(
            config["endpoint"].As<std::string>())) {
              bts.AsyncDetach("task_watch", [&](){
                while(!userver::engine::current_task::ShouldCancel()){
                  Reset();
                }
              });
            }

auto WatchClient::Init(){
  ::etcdserverpb::WatchRequest request;
  etcdserverpb::WatchCreateRequest create_request;
  create_request.set_key(/*key*/ "f");
  create_request.set_range_end(/*range_end*/"foobar");
  *request.mutable_create_request() = create_request;

  auto context = std::make_unique<grpc::ClientContext>();
  auto stream = grpc_watch_client_.Watch(std::move(context));
  stream.Write(request);

  etcdserverpb::WatchResponse response; 
  if(!stream.Read(response) || !response.created()){
    started = false;
    to_reset = true;
  }

  watch_id = response.watch_id();
  started = true;
  to_reset = false;
  return stream;
}

bool WatchClient::Reset(/*std::string& key, std::string& range_end*/){
  // while(callback not set)
  //std::string key = "#", range_end = "$";

  auto stream = Init();
  etcdserverpb::WatchResponse response; 

  while(!userver::engine::current_task::ShouldCancel()){
    while(!watch_callback){
      
    }

    if(to_reset){
      //should be commented
      if(!Destroy(stream)){
        continue;
      }
      stream = Init();
    }

    if(!stream.Read(response)){
      to_reset = true;
    }

    for(auto& i : *response.mutable_events()){   
      watch_callback(i.type() == mvccpb::Event_EventType::Event_EventType_PUT, i.kv().key(), i.kv().value());
      LOG_CRITICAL() << "reported event";
      //func(""); // process single event 
    }
  }
  return Destroy(stream);
}

bool WatchClient::Destroy(userver::ugrpc::client::BidirectionalStream<::etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>& stream){
  if(!started){
    return true;
  }
  // problem with writing to dead stream?
  etcdserverpb::WatchRequest second_request;
  etcdserverpb::WatchCancelRequest cancel_request;
  cancel_request.set_watch_id(watch_id);
  second_request.set_allocated_cancel_request(&cancel_request);
  stream.Write(second_request);
  stream.WritesDone();

  etcdserverpb::WatchResponse response; 
  if(!stream.Read(response) || !response.canceled()){
    return false;
  }

  stream.Finish();
  return true;
}

WatchClient::~WatchClient() {
  bts.CancelAndWait();
}

void WatchClient::SetCallback(std::function<void(bool, std::string, std::string)> func) {
  watch_callback = std::move(func);
}

} // namespace storages::etcd

USERVER_NAMESPACE_END
