#include "quickfix_fix/message.hpp"

#include "lab/error.hpp"
#include "quickfix_fix/errors.hpp"
#include "quickfix_fix/session.hpp"

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

void memory_session::connect(memory_session& peer)
{
  peer_ = &peer;
}

lab::result<void> memory_session::send(const message& outbound_message)
{
  if (peer_ == nullptr) {
    return lab::make_leaf_error(errors::not_connected{});
  }

  peer_->receive(outbound_message);
  return {};
}

void memory_session::receive(const message& inbound_message)
{
  if (on_message) {
    on_message(inbound_message);
  }
}

void memory_session::receive_reject(std::string_view reason)
{
  if (on_reject) {
    on_reject(reason);
  }
}

session_pair::session_pair()
{
  initiator.connect(acceptor);
  acceptor.connect(initiator);
}

} // namespace quickfix_fix
