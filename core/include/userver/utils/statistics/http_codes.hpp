#include <memory>
#include <unordered_map>

#include <utils/statistics/relaxed_counter.hpp>

namespace formats::json {
class Value;
}

namespace utils::statistics {

class HttpCodes final {
 public:
  using CodeType = unsigned short;

  HttpCodes() = default;
  explicit HttpCodes(std::initializer_list<CodeType> special_interest_codes);

  void Account(CodeType code);

  std::size_t Codes1xx() const { return code_1xx; }
  std::size_t Codes2xx() const { return code_2xx; }
  std::size_t Codes3xx() const { return code_3xx; }
  std::size_t Codes4xx() const { return code_4xx; }
  std::size_t Codes5xx() const { return code_5xx; }
  std::size_t CodesOther() const { return code_other; }
  std::size_t SpecialInterestCode(CodeType code) const {
    return particular_codes.at(code);
  }

  formats::json::Value FormatReplyCodes() const;

 private:
  using ValueType = std::uint64_t;
  using Counter = RelaxedCounter<ValueType>;

  Counter code_1xx;
  Counter code_2xx;
  Counter code_3xx;
  Counter code_4xx;
  Counter code_5xx;
  Counter code_other;

  std::unordered_map<unsigned short, Counter> particular_codes;
};

}  // namespace utils::statistics
