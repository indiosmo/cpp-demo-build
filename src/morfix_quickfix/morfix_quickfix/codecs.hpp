#ifndef MORFIX_QUICKFIX_CODECS_HPP
#define MORFIX_QUICKFIX_CODECS_HPP

#include "lab/result.hpp"
#include "morfix/messages.hpp"
#include "quickfix_fix/message.hpp"

namespace morfix_quickfix {

class initiator_codec
{
public:
  virtual ~initiator_codec() = default;

  virtual lab::result<quickfix_fix::message> encode(const morfix::request& request) const = 0;
  virtual lab::result<morfix::event> decode(const quickfix_fix::message& message) const = 0;
};

class acceptor_codec
{
public:
  virtual ~acceptor_codec() = default;

  virtual lab::result<quickfix_fix::message> encode(const morfix::event& event) const = 0;
  virtual lab::result<morfix::request> decode(const quickfix_fix::message& message) const = 0;
};

namespace codecs::b3 {

class initiator final : public initiator_codec
{
public:
  lab::result<quickfix_fix::message> encode(const morfix::request& request) const override;
  lab::result<morfix::event> decode(const quickfix_fix::message& message) const override;
};

class acceptor final : public acceptor_codec
{
public:
  lab::result<quickfix_fix::message> encode(const morfix::event& event) const override;
  lab::result<morfix::request> decode(const quickfix_fix::message& message) const override;
};

} // namespace codecs::b3

} // namespace morfix_quickfix

#endif /* MORFIX_QUICKFIX_CODECS_HPP */
