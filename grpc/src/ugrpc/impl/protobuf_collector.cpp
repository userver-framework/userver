#include <userver/ugrpc/impl/protobuf_collector.hpp>

#include <unordered_set>

#include <google/protobuf/descriptor.h>

#include <userver/ugrpc/protobuf_visit.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

std::unordered_set<std::string>& GetGeneratedMessagesImpl() {
  static std::unordered_set<std::string> messages;
  return messages;
}

}  // namespace

void RegisterMessageTypes(std::initializer_list<std::string> type_names) {
  utils::impl::AssertStaticRegistrationAllowed(
      "Calling ugrpc::impl::RegisterMessageTypes()");
  GetGeneratedMessagesImpl().merge(std::unordered_set<std::string>(type_names));
}

DescriptorList GetGeneratedMessages() {
  utils::impl::AssertStaticRegistrationFinished();

  DescriptorList result;
  for (const std::string& service_name : GetGeneratedMessagesImpl()) {
    const google::protobuf::Descriptor* descriptor =
        ugrpc::FindGeneratedMessage(service_name);
    UINVARIANT(descriptor, "descriptor is nullptr");
    result.push_back(descriptor);
  }
  return result;
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
