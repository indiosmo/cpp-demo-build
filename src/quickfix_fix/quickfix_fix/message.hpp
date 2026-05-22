#ifndef QUICKFIX_FIX_MESSAGE_HPP
#define QUICKFIX_FIX_MESSAGE_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace quickfix_fix {

using tag = std::uint16_t;

struct field
{
  tag number;
  std::string value;
};

class message
{
public:
  explicit message(std::string msg_type);

  [[nodiscard]] std::string_view msg_type() const;

  void set(tag number, std::string value);

  [[nodiscard]] std::optional<std::string_view> get(tag number) const;
  [[nodiscard]] const std::vector<field>& fields() const;

private:
  std::string msg_type_;
  std::vector<field> fields_;
};

} // namespace quickfix_fix

#endif /* QUICKFIX_FIX_MESSAGE_HPP */
