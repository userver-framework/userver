#pragma once

#include <curl-ev/form.hpp>

namespace clients {
namespace http {

class Form final : public curl::form {
  using curl::form::form;
};

}  // namespace http
}  // namespace clients
