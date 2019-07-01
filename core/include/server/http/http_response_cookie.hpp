#pragma once

#include <chrono>
#include <memory>
#include <string>

namespace server::http {

class Cookie final {
 public:
  Cookie(std::string name, std::string value);
  Cookie(Cookie&& cookie) noexcept;
  Cookie(const Cookie& cookie);
  ~Cookie() noexcept;

  Cookie& operator=(Cookie&&) noexcept;
  Cookie& operator=(const Cookie& cookie);

  const std::string& Name() const;
  const std::string& Value() const;

  bool IsSecure() const;
  Cookie& SetSecure();

  std::chrono::system_clock::time_point Expires() const;
  Cookie& SetExpires(std::chrono::system_clock::time_point value);

  bool IsPermanent() const;
  Cookie& SetPermanent();

  bool IsHttpOnly() const;
  Cookie& SetHttpOnly();

  const std::string& Path() const;
  Cookie& SetPath(std::string value);

  const std::string& Domain() const;
  Cookie& SetDomain(std::string value);

  std::chrono::seconds MaxAge() const;
  Cookie& SetMaxAge(std::chrono::seconds value);

  std::string ToString() const;

 private:
  class CookieData;
  std::unique_ptr<CookieData> data_;
};

}  // namespace server::http
