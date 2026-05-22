#ifndef QUICKFIX_FIX_SESSION_HPP
#define QUICKFIX_FIX_SESSION_HPP

#include "lab/inplace_function.hpp"
#include "lab/json.hpp"
#include "lab/result.hpp"
#include "quickfix_fix/message.hpp"

#include <string>

namespace quickfix_fix {

struct session_pair_config
{
  std::string initiator_session_id;
  std::string acceptor_session_id;
  bool in_memory_delivery;
};

LAB_AUTO_JSON(session_pair_config, initiator_session_id, acceptor_session_id, in_memory_delivery)

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

class memory_session final : public session
{
public:
  void connect(memory_session& peer);

  lab::result<void> send(const message& message) override;
  void receive(const message& message);
  void receive_reject(std::string_view reason);

private:
  memory_session* peer_{};
};

struct session_pair
{
  session_pair();

  memory_session initiator;
  memory_session acceptor;
};

} // namespace quickfix_fix

#endif /* QUICKFIX_FIX_SESSION_HPP */
