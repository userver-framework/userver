
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
    userver::server::request::RequestContext& context) const {

    auto user_id = context.GetData<std::string>("id");
    const auto& comment_id = std::atoi(request.GetPathArg("id").c_str());

    const auto result_find_comment = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                            sql::kSelectCommentById.data(),
                            comment_id);
    
    if(result_find_comment.IsEmpty()){
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound); 
        return "NOT_FOUND_COMMENT";
    }

    const auto result_delete_comment = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                            sql::kDeleteCommentById.data(),
                            comment_id, user_id);

    if(result_delete_comment.IsEmpty()){
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed ); 
        return "USER_NOT_OWNER";
    }
    
                            
    return "";

}

} // namespace real_medium::handlers::comments::del