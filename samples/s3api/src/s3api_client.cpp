#include "s3api_client.hpp"

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

/// [includes]

#include <userver/s3api/authenticators/access_key.hpp>
#include <userver/s3api/clients/s3api.hpp>
#include <userver/s3api/models/s3api_connection_type.hpp>

/// [includes]

namespace samples {

S3ApiSampleComponent::S3ApiSampleComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : LoggableComponentBase(config, context),
      url_(config["url"].As<std::string>()),
      http_client_(context.FindComponent<::components::HttpClient>().GetHttpClient())
{
    auto& testsuite_tasks = testsuite::GetTestsuiteTasks(context);
    // Only register task for testsuite environment
    if (testsuite_tasks.IsEnabled()) {
        testsuite_tasks.RegisterTask("some-s3-fill-task", [this] {
            auto my_client = GetClient();
            DoVeryImportantThingsInS3(std::move(my_client));
        });
    }
}

/// [create_client]

s3api::ClientPtr S3ApiSampleComponent::GetClient() {
    // Create connection settings
    auto connection_cfg = s3api::ConnectionCfg(
        std::chrono::milliseconds{100}, /* timeout */
        2,                              /* retries */
        std::nullopt                    /* proxy */
    );

    // Create connection object
    auto s3_connection = s3api::MakeS3Connection(http_client_, s3api::S3ConnectionType::kHttps, url_, connection_cfg);

    // Create authorizer.
    auto auth = std::make_shared<s3api::authenticators::AccessKey>("my-access-key", s3api::Secret("my-secret-key"));

    // create and return client
    return s3api::GetS3Client(std::move(s3_connection), std::move(auth), "mybucket");
}

/// [create_client]

yaml_config::Schema S3ApiSampleComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: S3 API sample component
additionalProperties: false
properties:
    url:
        type: string
        description: URL to S3 API
)");
}

/// [using_client]

void DoVeryImportantThingsInS3(s3api::ClientPtr client) {
    std::string path = "path/to/object";
    std::string data{"some string data from S3ApiSampleComponent start"};

    client->PutObject(path, std::move(data));
    client->GetObject(path);
}

/// [using_client]

}  // namespace samples
