#include "market_data/publisher.hpp"

#include "market_data/encoder.hpp"
#include "market_data/messages.hpp"
#include "market_data/sink.hpp"

namespace market_data {

publisher::publisher(encoder& msg_encoder, sink& output_sink)
  : encoder_{msg_encoder}
  , sink_{output_sink}
{
}

void publisher::send(const message& msg)
{
  sink_.write(encoder_.encode(msg));
}

} // namespace market_data
