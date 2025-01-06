#include <userver/clients/http/component.hpp>
#include <userver/components/loggable_component_base.hpp>

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/s3api/clients/fwd.hpp>

namespace samples {

/// [s3_sample_component]

// This component is not required to create S3 api client. It is used for demonstration
// purposes only.
class S3ApiSampleComponent : public components::LoggableComponentBase {
public:
    // `kName` is used as the component name in static config
    static constexpr std::string_view kName = "s3-sample-component";

    S3ApiSampleComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    s3api::ClientPtr GetClient();

private:
    // http_client MUST outlive any dependent s3 client
    clients::http::Client& http_client_;
};

/// [s3_sample_component]

void DoVeryImportantThingsInS3(s3api::ClientPtr client);

}  // namespace samples
