#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/text_light.hpp>

namespace samples {

/// [Multipart service sample - component]
class Multipart final : public server::handlers::HttpHandlerBase {
 public:
  // `kName` is used as the component name in static config
  static constexpr std::string_view kName = "handler-multipart-sample";

  // Component is valid after construction and is able to accept requests
  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& req,
      server::request::RequestContext&) const override;
};
/// [Multipart service sample - component]

/// [Multipart service sample - HandleRequestThrow]
std::string Multipart::HandleRequestThrow(
    const server::http::HttpRequest& req,
    server::request::RequestContext&) const {
  const auto content_type =
      http::ContentType(req.GetHeader(http::headers::kContentType));
  if (content_type != "multipart/form-data") {
    req.GetHttpResponse().SetStatus(server::http::HttpStatus::kBadRequest);
    return "Expected 'multipart/form-data' content type";
  }

  const auto& image = req.GetFormDataArg("profileImage");
  static constexpr std::string_view kPngMagicBytes = "\x89PNG\r\n\x1a\n";
  if (!utils::text::StartsWith(image.value, kPngMagicBytes)) {
    req.GetHttpResponse().SetStatus(server::http::HttpStatus::kBadRequest);
    return "Expecting PNG image format";
  }

  const auto& address = req.GetFormDataArg("address");
  auto json_addr = formats::json::FromString(address.value);

  return fmt::format("city={} image_size={}",
                     json_addr["city"].As<std::string>(), image.value.size());
}
/// [Multipart service sample - HandleRequestThrow]

}  // namespace samples

/// [Multipart service sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList().Append<samples::Multipart>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Multipart service sample - main]
