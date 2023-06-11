#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>

#include <userver/utest/using_namespace_userver.hpp>

/// [Flatbuf service sample - component]
#include <userver/server/handlers/http_handler_flatbuf_base.hpp>
#include "flatbuffer_schema.fbs.h"

namespace samples::fbs_handle {

class FbsSumEcho final
    : public server::handlers::HttpHandlerFlatbufBase<fbs::SampleRequest,
                                                      fbs::SampleResponse> {
 public:
  // `kName` is used as the component name in static config
  static constexpr std::string_view kName = "handler-fbs-sample";

  // Component is valid after construction and is able to accept requests
  FbsSumEcho(const components::ComponentConfig& config,
             const components::ComponentContext& context)
      : HttpHandlerFlatbufBase(config, context) {}

  fbs::SampleResponse::NativeTableType HandleRequestFlatbufThrow(
      const server::http::HttpRequest& /*request*/,
      const fbs::SampleRequest::NativeTableType& fbs_request,
      server::request::RequestContext&) const override {
    fbs::SampleResponse::NativeTableType res;
    res.sum = fbs_request.arg1 + fbs_request.arg2;
    res.echo = fbs_request.data;
    return res;
  }
};

}  // namespace samples::fbs_handle
/// [Flatbuf service sample - component]

namespace samples::fbs_request {

/// [Flatbuf service sample - http component]
class FbsRequest final : public components::LoggableComponentBase {
 public:
  static constexpr auto kName = "fbs-request";

  FbsRequest(const components::ComponentConfig& config,
             const components::ComponentContext& context)
      : LoggableComponentBase(config, context),
        http_client_{
            context.FindComponent<components::HttpClient>().GetHttpClient()},
        task_{utils::Async("requests", [this]() { KeepRequesting(); })} {}

  void KeepRequesting() const;

 private:
  clients::http::Client& http_client_;
  engine::TaskWithResult<void> task_;
};
/// [Flatbuf service sample - http component]

/// [Flatbuf service sample - request]
void FbsRequest::KeepRequesting() const {
  // Fill the payload data
  fbs::SampleRequest::NativeTableType payload;
  payload.arg1 = 20;
  payload.arg2 = 22;
  payload.data = "Hello word";

  // Serialize the payload into a std::string
  flatbuffers::FlatBufferBuilder fbb;
  auto ret_fbb = fbs::SampleRequest::Pack(fbb, &payload);
  fbb.Finish(ret_fbb);
  std::string data(reinterpret_cast<const char*>(fbb.GetBufferPointer()),
                   fbb.GetSize());

  // Send it
  const auto response = http_client_.CreateRequest()
                            .post("http://localhost:8084/fbs", std::move(data))
                            .timeout(std::chrono::seconds(1))
                            .retry(10)
                            .perform();

  // Response code should be 200 (use proper error handling in real code!)
  UASSERT_MSG(response->IsOk(), "Sample should work well in tests");

  // Verify and deserialize response
  const auto body = response->body_view();
  const auto* response_fb =
      flatbuffers::GetRoot<fbs::SampleResponse>(body.data());
  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(body.data()),
                                 body.size());
  UASSERT_MSG(response_fb->Verify(verifier), "Broken flatbuf in sample");
  fbs::SampleResponse::NativeTableType result;
  response_fb->UnPackTo(&result);

  // Make sure that the response is the expected one for sample
  UASSERT_MSG(result.sum == 42, "Sample should work well in tests");
  UASSERT_MSG(result.echo == payload.data, "Sample should work well in tests");
}
/// [Flatbuf service sample - request]

}  // namespace samples::fbs_request

int main(int argc, char* argv[]) {
  auto component_list = components::MinimalServerComponentList()        //
                            .Append<samples::fbs_handle::FbsSumEcho>()  //

                            .Append<clients::dns::Component>()            //
                            .Append<components::HttpClient>()             //
                            .Append<samples::fbs_request::FbsRequest>();  //
  return utils::DaemonMain(argc, argv, component_list);
}
