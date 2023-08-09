
#include "comment_delete.hpp"
#include "db/sql.hpp"

namespace real_medium::handlers::comments::del {

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
    check authentication
    ..
    */
    
    
    const auto& id_comment = std::atoi(request.GetPathArg("id").c_str());


    const auto result =  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                            sql::kDeleteCommentById.data(),
                            id_comment);

    if(result.IsEmpty()){
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound); 
        return "NOT_FOUND_COMMENT";
    }
    
                            
    return "";

}

} // namespace real_medium::handlers::comments::del