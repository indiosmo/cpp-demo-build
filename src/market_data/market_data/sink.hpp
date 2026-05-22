#ifndef MARKET_DATA_SINK_HPP
#define MARKET_DATA_SINK_HPP

#include <string_view>

/*
 * Abstract sink boundary: takes an already-encoded record and writes it to
 * the chosen target. Lets the wiring layer pick stdout, a file, or another
 * backend without the encoder caring.
 */

namespace market_data {

class sink
{
public:
  virtual ~sink() = default;

  virtual void write(std::string_view record) = 0;
};

} // namespace market_data

#endif /* MARKET_DATA_SINK_HPP */
