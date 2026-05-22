#ifndef MARKET_DATA_PUBLISHER_HPP
#define MARKET_DATA_PUBLISHER_HPP

#include "market_data/encoder.hpp"
#include "market_data/messages.hpp"
#include "market_data/sink.hpp"

/*
 * Pipeline stage that drives the encode-then-write step. Holds references
 * to an encoder and a sink supplied by the wiring layer, keeping this a
 * pure synchronous transform that tests can rewire by swapping either side.
 */

namespace market_data {

class publisher
{
public:
  publisher(encoder& msg_encoder, sink& output_sink);

  void send(const message& msg);

private:
  encoder& encoder_;
  sink& sink_;
};

} // namespace market_data

#endif /* MARKET_DATA_PUBLISHER_HPP */
