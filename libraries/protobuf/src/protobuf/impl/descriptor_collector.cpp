#include <userver/protobuf/impl/descriptor_collector.hpp>

#include <unordered_set>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::impl {

namespace {

std::unordered_set<std::string>& GetGeneratedMessagesImpl() {
    static std::unordered_set<std::string> messages;
    return messages;
}

}  // namespace

void RegisterMessageTypes(std::initializer_list<std::string> type_names) {
    utils::impl::AssertStaticRegistrationAllowed("Calling protobuf::impl::RegisterMessageTypes()");
    GetGeneratedMessagesImpl().merge(std::unordered_set<std::string>(type_names));
}

const google::protobuf::Descriptor* FindGeneratedMessage(std::string_view name) {
    const google::protobuf::DescriptorPool* pool = google::protobuf::DescriptorPool::generated_pool();
    UINVARIANT(pool, "pool is nullptr");
#if GOOGLE_PROTOBUF_VERSION >= 4022000
    return pool->FindMessageTypeByName(name);
#else
    return pool->FindMessageTypeByName(std::string(name));
#endif
}

std::vector<const google::protobuf::Descriptor*> GetGeneratedMessages() {
    utils::impl::AssertStaticRegistrationFinished();

    const auto& generated_messages = GetGeneratedMessagesImpl();
    std::vector<const google::protobuf::Descriptor*> result;
    result.reserve(generated_messages.size());
    for (const std::string& message_name : generated_messages) {
        const google::protobuf::Descriptor* descriptor = FindGeneratedMessage(message_name);
        UINVARIANT(descriptor, "descriptor is nullptr");
        result.push_back(descriptor);
    }
    return result;
}

}  // namespace protobuf::impl

USERVER_NAMESPACE_END
