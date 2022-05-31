#pragma once

/// @file userver/server/handlers/http_handler_flatbuf_base.hpp
/// @brief @copybrief server::handlers::HttpHandlerFlatbufBase

#include <type_traits>

#include <flatbuffers/flatbuffers.h>

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/utils/log.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace impl {

inline const std::string kFlatbufRequestDataName = "__request_flatbuf";
inline const std::string kFlatbufResponseDataName = "__response_flatbuf";

}  // namespace impl

// clang-format off

/// @ingroup userver_components userver_http_handlers userver_base_classes
///
/// @brief Convenient base for handlers that accept requests with body in
/// Flatbuffer format and respond with body in Flatbuffer format.
///
/// ## Example usage:
///
/// @snippet samples/flatbuf_service/flatbuf_service.cpp Flatbuf service sample - component

// clang-format on

template <typename InputType, typename ReturnType>
class HttpHandlerFlatbufBase : public HttpHandlerBase {
  static_assert(std::is_base_of<flatbuffers::Table, InputType>::value,
                "Input type should be auto-generated FlatBuffers table type");
  static_assert(std::is_base_of<flatbuffers::Table, ReturnType>::value,
                "Return type should be auto-generated FlatBuffers table type");

 public:
  HttpHandlerFlatbufBase(const components::ComponentConfig& config,
                         const components::ComponentContext& component_context);

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext& context) const final;

  virtual typename ReturnType::NativeTableType HandleRequestFlatbufThrow(
      const http::HttpRequest& request,
      const typename InputType::NativeTableType& input,
      request::RequestContext& context) const = 0;

  /// @returns A pointer to input data if it was parsed successfully or
  /// nullptr otherwise.
  const typename InputType::NativeTableType* GetInputData(
      const request::RequestContext& context) const;

  /// @returns a pointer to output data if it was returned successfully by
  /// `HandleRequestFlatbufThrow()` or nullptr otherwise.
  const typename ReturnType::NativeTableType* GetOutputData(
      const request::RequestContext& context) const;

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  /// Override it if you need a custom request body logging.
  std::string GetRequestBodyForLogging(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& request_body) const override;

  /// Override it if you need a custom response data logging.
  std::string GetResponseDataForLogging(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& response_data) const override;

  void ParseRequestData(const http::HttpRequest& request,
                        request::RequestContext& context) const final;
};

template <typename InputType, typename ReturnType>
HttpHandlerFlatbufBase<InputType, ReturnType>::HttpHandlerFlatbufBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context) {}

template <typename InputType, typename ReturnType>
std::string HttpHandlerFlatbufBase<InputType, ReturnType>::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  const auto& input =
      context.GetData<const typename InputType::NativeTableType&>(
          impl::kFlatbufRequestDataName);

  const auto& ret = context.SetData<const typename ReturnType::NativeTableType>(
      impl::kFlatbufResponseDataName,
      HandleRequestFlatbufThrow(request, input, context));

  flatbuffers::FlatBufferBuilder fbb;
  auto ret_fbb = ReturnType::Pack(fbb, &ret);
  fbb.Finish(ret_fbb);
  return {reinterpret_cast<const char*>(fbb.GetBufferPointer()), fbb.GetSize()};
}

template <typename InputType, typename ReturnType>
const typename InputType::NativeTableType*
HttpHandlerFlatbufBase<InputType, ReturnType>::GetInputData(
    const request::RequestContext& context) const {
  return context.GetDataOptional<const typename InputType::NativeTableType>(
      impl::kFlatbufRequestDataName);
}

template <typename InputType, typename ReturnType>
const typename ReturnType::NativeTableType*
HttpHandlerFlatbufBase<InputType, ReturnType>::GetOutputData(
    const request::RequestContext& context) const {
  return context.GetDataOptional<const typename ReturnType::NativeTableType>(
      impl::kFlatbufResponseDataName);
}

template <typename InputType, typename ReturnType>
std::string
HttpHandlerFlatbufBase<InputType, ReturnType>::GetRequestBodyForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& request_body) const {
  size_t limit = GetConfig().request_body_size_log_limit;
  return utils::log::ToLimitedHex(request_body, limit);
}

template <typename InputType, typename ReturnType>
std::string
HttpHandlerFlatbufBase<InputType, ReturnType>::GetResponseDataForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& response_data) const {
  size_t limit = GetConfig().response_data_size_log_limit;
  return utils::log::ToLimitedHex(response_data, limit);
}

template <typename InputType, typename ReturnType>
void HttpHandlerFlatbufBase<InputType, ReturnType>::ParseRequestData(
    const http::HttpRequest& request, request::RequestContext& context) const {
  const auto& body = request.RequestBody();
  const auto* input_fbb = flatbuffers::GetRoot<InputType>(body.data());
  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(body.data()),
                                 body.size());
  if (!input_fbb->Verify(verifier)) {
    throw ClientError(
        InternalMessage{"Invalid FlatBuffers format in request body"});
  }

  typename InputType::NativeTableType input;
  input_fbb->UnPackTo(&input);

  context.SetData(impl::kFlatbufRequestDataName, std::move(input));
}

template <typename InputType, typename ReturnType>
yaml_config::Schema
HttpHandlerFlatbufBase<InputType, ReturnType>::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("HTTP handler flatbuf base config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
