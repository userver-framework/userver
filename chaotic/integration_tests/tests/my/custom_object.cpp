#include <userver/chaotic/io/my/custom_object.hpp>

#include <schemas/custom_cpp_type.hpp>

namespace my {

CustomObject::CustomObject(ns::CustomObject&& obj) : foo(obj.foo) {}

ns::CustomObject Convert(
    const CustomObject& value,
    USERVER_NAMESPACE::chaotic::convert::To<ns::CustomObject>) {
  ns::CustomObject obj;
  obj.foo = value.foo;
  return obj;
}

}  // namespace my
