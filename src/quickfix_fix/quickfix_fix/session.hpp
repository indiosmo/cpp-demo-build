#ifndef QUICKFIX_FIX_SESSION_HPP
#define QUICKFIX_FIX_SESSION_HPP

#include "lab/inplace_function.hpp"
#include "lab/result.hpp"
#include "quickfix_fix/message.hpp"

#include <string>

namespace quickfix_fix {

using message_callback = lab::inplace_function<void(const message&), 128>;
using reject_callback = lab::inplace_function<void(std::string_view), 128>;

class session
{
public:
  virtual ~session() = default;

  message_callback on_message;
  reject_callback on_reject;

  virtual lab::result<void> send(const message& message) = 0;
};

} // namespace quickfix_fix

#endif /* QUICKFIX_FIX_SESSION_HPP */
