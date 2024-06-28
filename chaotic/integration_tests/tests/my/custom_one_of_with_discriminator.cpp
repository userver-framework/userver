#include <userver/chaotic/io/my/custom_one_of_with_discriminator.hpp>

#include <userver/utils/overloaded.hpp>

#include <schemas/custom_cpp_type.hpp>

namespace my {

std::variant<ns::CustomStruct1, ns::CustomStruct2> Convert(
    const CustomOneOfWithDiscriminator& value,
    USERVER_NAMESPACE::chaotic::convert::To<
        std::variant<ns::CustomStruct1, ns::CustomStruct2>>) {
  return std::visit(
      USERVER_NAMESPACE::utils::Overloaded{
          [](int x) -> std::variant<ns::CustomStruct1, ns::CustomStruct2> {
            return ns::CustomStruct1{"CustomStruct1", x};
          },
          [](const std::string& x)
              -> std::variant<ns::CustomStruct1, ns::CustomStruct2> {
            return ns::CustomStruct2{"CustomStruct2", x};
          },
      },
      value.val);
}

}  // namespace my
