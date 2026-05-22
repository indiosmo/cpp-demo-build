#include "quickfix_fix/message.hpp"

#include <utility>

namespace quickfix_fix {

message::message(std::string msg_type)
  : msg_type_{std::move(msg_type)}
{
}

std::string_view message::msg_type() const
{
  return msg_type_;
}

void message::set(tag number, std::string value)
{
  for (auto& field : fields_) {
    if (field.number == number) {
      field.value = std::move(value);
      return;
    }
  }

  fields_.push_back(field{.number = number, .value = std::move(value)});
}

std::optional<std::string_view> message::get(tag number) const
{
  for (const auto& field : fields_) {
    if (field.number == number) {
      return field.value;
    }
  }
  return std::nullopt;
}

const std::vector<field>& message::fields() const
{
  return fields_;
}

} // namespace quickfix_fix
