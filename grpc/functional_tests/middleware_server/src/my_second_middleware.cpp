#include "my_second_middleware.hpp"

namespace functional_tests {

void MySecondMiddleware::PostRecvMessage(
    ugrpc::server::MiddlewareCallContext& context,
    google::protobuf::Message& request
) const {
    const google::protobuf::Descriptor* descriptor = request.GetDescriptor();
    const google::protobuf::FieldDescriptor* name_field = descriptor->FindFieldByName("name");
    UINVARIANT(name_field->type() == google::protobuf::FieldDescriptor::TYPE_STRING, "field must be a string");
    if (name_field) {
        const google::protobuf::Reflection* reflection = request.GetReflection();
        auto name = reflection->GetString(request, name_field);
        name += " Two";
        reflection->SetString(&request, name_field, name);
    } else {
        return context.SetError(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Field 'name' not found"));
    }
}

void MySecondMiddleware::PreSendMessage(
    ugrpc::server::MiddlewareCallContext& context,
    google::protobuf::Message& response
) const {
    const google::protobuf::Descriptor* descriptor = response.GetDescriptor();
    const google::protobuf::FieldDescriptor* name_field = descriptor->FindFieldByName("greeting");
    UINVARIANT(name_field->type() == google::protobuf::FieldDescriptor::TYPE_STRING, "field must be a string");
    if (name_field) {
        const google::protobuf::Reflection* reflection = response.GetReflection();
        auto greeting = reflection->GetString(response, name_field);
        greeting += " EndTwo";
        reflection->SetString(&response, name_field, greeting);
    } else {
        return context.SetError(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Field 'greeting' not found"));
    }
}

std::shared_ptr<const ugrpc::server::MiddlewareBase>
MySecondMiddlewareComponent::CreateMiddleware(const ugrpc::server::ServiceInfo&, const yaml_config::YamlConfig&) const {
    return middleware_;
}

}  // namespace functional_tests
