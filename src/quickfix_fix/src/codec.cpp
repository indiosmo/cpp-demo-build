#include "quickfix_fix/codec.hpp"

#include "lab/charconv.hpp"
#include "lab/error.hpp"
#include "lab/fmt.hpp"
#include "quickfix_fix/errors.hpp"

#include <utility>

namespace quickfix_fix {
namespace {

constexpr tag msg_type_tag = 35;

lab::result<void> decode_field(std::string_view field, std::string& msg_type, message& output)
{
  const auto separator = field.find('=');
  if (separator == std::string_view::npos || separator == 0) {
    return lab::make_leaf_error(errors::malformed_message{.text = fmt::format("malformed FIX field '{}'", field)});
  }

  BOOST_LEAF_ASSIGN(auto number, lab::from_chars<tag>(field.substr(0, separator)));
  const auto value = field.substr(separator + 1);

  if (number == msg_type_tag) {
    msg_type = std::string{value};
    return {};
  }

  output.set(number, std::string{value});
  return {};
}

} // namespace

std::string encode(const message& message, char delimiter)
{
  auto output = fmt::format("{}={}{}", msg_type_tag, message.msg_type(), delimiter);

  for (const auto& field : message.fields()) {
    output += fmt::format("{}={}{}", field.number, field.value, delimiter);
  }

  return output;
}

lab::result<message> decode(std::string_view payload, char delimiter)
{
  std::string msg_type;
  message output{""};

  while (!payload.empty()) {
    const auto end = payload.find(delimiter);
    const auto field = payload.substr(0, end);

    if (!field.empty()) {
      LAB_LEAF_CHECK(decode_field(field, msg_type, output));
    }

    if (end == std::string_view::npos) {
      break;
    }
    payload.remove_prefix(end + 1);
  }

  if (msg_type.empty()) {
    return lab::make_leaf_error(errors::missing_msg_type{});
  }

  message typed_output{std::move(msg_type)};
  for (const auto& field : output.fields()) {
    typed_output.set(field.number, field.value);
  }

  return typed_output;
}

} // namespace quickfix_fix
