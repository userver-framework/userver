
#include "user_get.hpp"
#include "../../models/user.hpp"
#include "../../dto/user.hpp"
#include "db/sql.hpp"

namespace real_medium::handlers::users::get {

Handler::Handler(const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context)
      , pg_cluster_(component_context
                        .FindComponent<userver::components::Postgres>(
                            "realmedium-database")
                        .GetCluster()) {}

std::string Handler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {

    /*
    ..
    get user_id by token
    ..
    */
    
    auto user_id = "3dff4620-c620-4372-9d34-8d44b6bbc041"; 

    const auto result =  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                            sql::kFindUserById.data(),
                            user_id);
    

    if(result.IsEmpty()){
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return "USER_NOT_FOUND";
    }

    auto user = result.AsSingleRow<real_medium::models::User>(userver::storages::postgres::kRowTag);

    userver::formats::json::ValueBuilder builder;
    builder["user"] = user;
                            
    return ToString(builder.ExtractValue());

}

} // namespace real_medium::handlers::users::get