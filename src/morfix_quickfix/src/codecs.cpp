#include "morfix_quickfix/codecs.hpp"

#include "lab/error.hpp"
#include "morfix_quickfix/errors.hpp"

namespace morfix_quickfix::codecs::b3 {

lab::result<quickfix_fix::message> initiator::encode(const morfix::request&) const
{
  return lab::make_leaf_error(errors::not_implemented{.operation = "B3 initiator request encoding"});
}

lab::result<morfix::event> initiator::decode(const quickfix_fix::message& message) const
{
  return lab::make_leaf_error(errors::unsupported_message{.message_type = std::string{message.msg_type()}});
}

lab::result<quickfix_fix::message> acceptor::encode(const morfix::event&) const
{
  return lab::make_leaf_error(errors::not_implemented{.operation = "B3 acceptor event encoding"});
}

lab::result<morfix::request> acceptor::decode(const quickfix_fix::message& message) const
{
  return lab::make_leaf_error(errors::unsupported_message{.message_type = std::string{message.msg_type()}});
}

} // namespace morfix_quickfix::codecs::b3
