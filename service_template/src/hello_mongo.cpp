// mongo template on
#include <hello_mongo.hpp>

#include <greeting.hpp>

#include <userver/formats/bson/inline.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/mongo/options.hpp>

static constexpr std::string_view kNameArg = "name";

namespace service_template {

HelloMongo::HelloMongo(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      mongo_pool_(component_context.FindComponent<userver::components::Mongo>("mongo-db-1").GetPool()) {}

std::string HelloMongo::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    using userver::formats::bson::MakeDoc;

    const auto& name = request.GetArg(kNameArg);
    auto user_type = UserType::kFirstTime;

    if (!name.empty()) {
        auto users = mongo_pool_->GetCollection("hello_users");
        const auto result = users.FindAndModify(
            MakeDoc(kNameArg, name), MakeDoc("$inc", MakeDoc("count", 1)), userver::storages::mongo::options::Upsert{}
        );

        if (result.ModifiedCount() > 0) {
            user_type = UserType::kKnown;
        }
    }

    return SayHelloTo(name, user_type);
}

}  // namespace service_template
// mongo template off
