#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/fs_cache.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/http_handler_static.hpp>
#include <userver/utils/daemon_run.hpp>

/// [Static service sample - Message]
struct Message {
    std::string name;
    std::string message;
};

Message Parse(formats::json::Value json, formats::parse::To<Message>) {
    Message result;
    result.name = json["name"].As<std::string>();
    result.message = json["message"].As<std::string>();
    return result;
}

formats::json::Value Serialize(const Message& value, formats::serialize::To<formats::json::Value>) {
    formats::json::ValueBuilder json;
    json["name"] = value.name;
    json["message"] = value.message;
    return json.ExtractValue();
}
/// [Static service sample - Message]

/// [Static service sample - MessagesHandler]
class MessagesHandler final : public server::handlers::HttpHandlerJsonBase {
public:
    using HttpHandlerJsonBase::HttpHandlerJsonBase;

    Value HandleRequestJsonThrow(const HttpRequest& request, const Value& json, RequestContext&) const override {
        if (request.GetMethod() == server::http::HttpMethod::kPost) {
            auto message = json.As<Message>();

            auto locked_data = messages_.Lock();
            locked_data->push_back(std::move(message));
            return {};
        }

        std::vector<Message> snapshot;
        {
            auto locked_data = messages_.Lock();
            snapshot = *locked_data;
        }
        return formats::json::ValueBuilder(snapshot).ExtractValue();
    }

private:
    mutable concurrent::Variable<std::vector<Message>> messages_;
};
/// [Static service sample - MessagesHandler]

/// [Static service sample - main]
int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<MessagesHandler>("handler-messages")
                                    .Append<components::FsCache>("fs-cache-main")
                                    .Append<server::handlers::HttpHandlerStatic>();
    return utils::DaemonMain(argc, argv, component_list);
}
/// [Static service sample - main]
