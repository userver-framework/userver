#pragma once

#include <type_traits>

#include <flatbuffers/flatbuffers.h>

#include <server/handlers/http_handler_base.hpp>
#include <server/http/http_error.hpp>

namespace server {
namespace handlers {

template <typename InputType, typename ReturnType>
class HttpHandlerFlatbufBase : public HttpHandlerBase {
  static_assert(std::is_base_of<flatbuffers::Table, InputType>::value,
                "Input type should be auto-generated FlatBuffers table type");
  static_assert(std::is_base_of<flatbuffers::Table, ReturnType>::value,
                "Return type should be auto-generated FlatBuffers table type");

 public:
  HttpHandlerFlatbufBase(const components::ComponentConfig& config,
                         const components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const override final;

  virtual typename ReturnType::NativeTableType HandleRequestFlatbufThrow(
      const http::HttpRequest& request,
      const typename InputType::NativeTableType& input,
      request::RequestContext& context) const = 0;
};

template <typename InputType, typename ReturnType>
HttpHandlerFlatbufBase<InputType, ReturnType>::HttpHandlerFlatbufBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context) {}

template <typename InputType, typename ReturnType>
std::string HttpHandlerFlatbufBase<InputType, ReturnType>::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  const auto& body = request.RequestBody();
  const auto* input_fbb = flatbuffers::GetRoot<InputType>(body.data());
  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(body.data()),
                                 body.size());
  if (!input_fbb->Verify(verifier)) {
    throw http::BadRequest("Invalid FlatBuffers format in request body");
  }

  typename InputType::NativeTableType input;
  input_fbb->UnPackTo(&input);

  auto ret = HandleRequestFlatbufThrow(request, input, context);

  flatbuffers::FlatBufferBuilder fbb;
  auto ret_fbb = ReturnType::Pack(fbb, &ret);
  fbb.Finish(ret_fbb);
  return {reinterpret_cast<const char*>(fbb.GetBufferPointer()), fbb.GetSize()};
}

}  // namespace handlers
}  // namespace server
