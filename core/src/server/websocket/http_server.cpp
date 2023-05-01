#include <userver/components/component.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/server/websocket/http_server.h>

#include <http_parser.h>

#include "content_encoder.h"
#include "utils.h"

namespace userver::websocket {

static HttpServer::Config::TlsConfig Parse(
    const userver::yaml_config::YamlConfig& value,
    userver::formats::parse::To<HttpServer::Config::TlsConfig>) {
  HttpServer::Config::TlsConfig config;

  std::string certfile = value["certfile"].As<std::string>();
  std::string keyfile = value["keyfile"].As<std::string>();
  config.cert = userver::crypto::Certificate::LoadFromString(
      fs::blocking::ReadFileContents(certfile));
  config.key = userver::crypto::PrivateKey::LoadFromString(
      fs::blocking::ReadFileContents(keyfile));

  return config;
}

static HttpServer::Config Parse(
    const userver::yaml_config::YamlConfig& value,
    userver::formats::parse::To<HttpServer::Config>) {
  HttpServer::Config config;
  config.allow_encoding = value["allow_encoding"].As<bool>();
  if (value.HasMember("tls"))
    config.tls = value["tls"].As<HttpServer::Config::TlsConfig>();
  return config;
}

yaml_config::Schema HttpServer::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::TcpAcceptorBase>(R"(
type: object
description: HTTP server.
additionalProperties: false
properties:
  allow_encoding:
    type: boolean
    description: Send encoded responses if client supports them.
  tls:
    type: object
    additionalProperties: false
    description: TLS support.
    properties:
      certfile:
        description: TLS certificate file.
        type: string
      keyfile:
        type: string
        description: TLS private key file.
)");
}

namespace {

HttpMethod ConvertHttpMethod(http_method method) {
  switch (method) {
    case HTTP_DELETE:
      return HttpMethod::kDelete;
    case HTTP_GET:
      return HttpMethod::kGet;
    case HTTP_HEAD:
      return HttpMethod::kHead;
    case HTTP_POST:
      return HttpMethod::kPost;
    case HTTP_PUT:
      return HttpMethod::kPut;
    case HTTP_CONNECT:
      return HttpMethod::kConnect;
    case HTTP_PATCH:
      return HttpMethod::kPatch;
    case HTTP_OPTIONS:
      return HttpMethod::kOptions;
    default:
      return HttpMethod::kUnknown;
  }
}

std::vector<char> status_response(HttpStatus status) {
  std::vector<char> result;
  fmt::format_to(std::back_inserter(result),
                 "HTTP/1.1 {} {}\r\nConnection: close\r\n\r\n", (int)status,
                 status);
  return result;
}

std::vector<std::byte> serialize_response(const Response& response,
                                          const Request& request,
                                          bool allow_encoding) {
  std::vector<std::byte> content;
  std::string additionalHeaders;
  if (!response.content_type.empty()) {
    if (allow_encoding) {
      if (auto acceptEncodingIt = request.headers.find("Accept-Encoding");
          acceptEncodingIt != request.headers.end()) {
        if (EncodeResult er =
                encode(make_span(response.content.data(), response.content.size()),
                       acceptEncodingIt->second);
            !er.encoded_data.empty()) {
          content = std::move(er.encoded_data);
          additionalHeaders += "Content-Encoding: ";
          additionalHeaders += er.encoding;
          additionalHeaders += "\r\n";
        }
      }
    }

    if (content.empty()) content = std::move(response.content);

    additionalHeaders += "Content-Type: ";
    additionalHeaders += response.content_type;
    additionalHeaders += "\r\n";
    additionalHeaders += "Content-Length: ";
    additionalHeaders += std::to_string(content.size());
    additionalHeaders += "\r\n";
  }
  if (!response.keepalive) additionalHeaders += "Connection: close\r\n";

  std::string head = "HTTP/1.1 ";
  head += std::to_string((int)response.status) + " " +
          ToString(response.status) + "\r\n";
  for (auto attrib : response.headers)
    head += attrib.first + ": " + attrib.second + "\r\n";
  head += additionalHeaders;
  head += "\r\n";

  std::vector<std::byte> result;
  result.reserve(head.size() + content.size());
  result.insert(result.end(), (const std::byte*)head.data(),
                (const std::byte*)(head.data() + head.size()));
  ;
  if (!content.empty())
    result.insert(result.end(), content.begin(), content.end());
  return result;
}

static const std::vector<char> throttled_answer =
    status_response(HttpStatus::kTooManyRequests);

struct ParserState {
  Request curRequest;

  std::string headerField;
  bool complete_header = false;
  bool complete = false;

  http_parser parser;
  http_parser_settings settings;

  ParserState() {
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;

    http_parser_settings_init(&settings);
    settings.on_message_begin = http_on_message_begin;
    settings.on_url = http_on_url;
    settings.on_header_field = http_on_header_field;
    settings.on_header_value = http_on_header_value;
    settings.on_headers_complete = http_on_header_complete;
    settings.on_body = http_on_body;
    settings.on_message_complete = http_on_message_complete;
  }

  static int http_on_url(http_parser* parser, const char* at, size_t length) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.url.append(at, length);
    return 0;
  }

  static int http_on_header_field(http_parser* parser, const char* at,
                                  size_t length) {
    auto self = static_cast<ParserState*>(parser->data);
    if (!self->complete_header)
      self->headerField.append(at, length);
    else {
      self->headerField.assign(at, length);
      self->complete_header = false;
    }
    return 0;
  }

  static int http_on_header_value(http_parser* parser, const char* at,
                                  size_t length) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.headers[self->headerField].append(at, length);
    self->complete_header = true;
    return 0;
  }

  static int http_on_body(http_parser* parser, const char* at, size_t length) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.content.insert(self->curRequest.content.end(),
                                    (const std::byte*)at,
                                    (const std::byte*)at + length);
    return 0;
  }

  static int http_on_message_begin(http_parser* parser) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.method = ConvertHttpMethod((http_method)parser->method);
    self->complete = false;
    return 0;
  }

  static int http_on_header_complete(http_parser* parser) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.keepalive = http_should_keep_alive(parser);
    return 0;
  }

  static int http_on_message_complete(http_parser* parser) {
    auto self = static_cast<ParserState*>(parser->data);
    self->complete = true;
    return 0;
  }
};

}  // namespace

HttpServer::HttpServer(const components::ComponentConfig& component_config,
                       const components::ComponentContext& component_context)
    : userver::components::TcpAcceptorBase(component_config, component_context),
      config(component_config.As<Config>()) {}

void HttpServer::ProcessSocket(engine::io::Socket&& sock) {
  std::unique_ptr<engine::io::RwBase> io;
  engine::io::Sockaddr remoteAddr = sock.Getpeername();
  if (!config.tls.has_value())
    io = std::make_unique<engine::io::Socket>(std::move(sock));
  else {
    HttpServer::Config::TlsConfig const& tlsConf = config.tls.value();
    io.reset(new engine::io::TlsWrapper(engine::io::TlsWrapper::StartTlsServer(
        std::move(sock), tlsConf.cert, tlsConf.key, engine::Deadline{}, {})));
  }

  std::array<std::byte, 512> buf;
  ParserState parserState;
  parserState.curRequest.client_address = &remoteAddr;

  while (!engine::current_task::ShouldCancel()) {
    size_t nread = io->ReadSome(buf.data(), buf.size(), {});
    if (nread > 0) {
      if (operation_mode == HttpServer::OperationMode::Throttled) {
        [[maybe_unused]] auto s =
            io->WriteAll(throttled_answer.data(), throttled_answer.size(), {});
        return;
      }

      http_parser_execute(&parserState.parser, &parserState.settings,
                          (const char*)buf.data(), nread);
      if (parserState.parser.http_errno != 0) {
        LOG_WARNING() << "bad data in http request ";
        return;
      }

      if (parserState.complete) {
        parserState.complete = false;

        Response response;
        std::optional<HttpStatus> errorStatus;

        Request curRequest = std::move(parserState.curRequest);
        try {
          response = HandleRequest(curRequest);
        } catch (HttpStatusException const& e) {
          errorStatus = e.status;
          LOG_INFO() << "Status exception " << ToString(e.status);
        } catch (std::exception const& e) {
          LOG_WARNING() << "Exception in http handler " << e;
          errorStatus = HttpStatus::kInternalServerError;
        }

        if (errorStatus.has_value()) {
          std::vector<char> respData = status_response(errorStatus.value());
          SendExactly(io.get(), as_bytes(utils::impl::Span(respData)), {});
          io.reset();
        } else {
          SendExactly(
              io.get(),
              serialize_response(response, curRequest, config.allow_encoding),
              {});
          if (response.post_send_cb) response.post_send_cb();
          if (response.upgrade_connection) {
            response.upgrade_connection(std::move(io));
            return;
          }
          if (!response.keepalive) return;
        }
      }

    } else if (nread == 0)  // connection closed
      return;
  }
}

void HttpServer::SetOperationMode(HttpServer::OperationMode opmode) {
  if (opmode == operation_mode) return;
  operation_mode = opmode;

  LOG_INFO() << "Http server operation mode changed to "
             << (opmode == OperationMode::Throttled ? "throttled" : "normal");
}

}  // namespace userver::websocket
