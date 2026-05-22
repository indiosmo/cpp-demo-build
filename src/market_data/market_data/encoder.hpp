#ifndef MARKET_DATA_ENCODER_HPP
#define MARKET_DATA_ENCODER_HPP

#include "market_data/messages.hpp"

#include <string>

/*
 * Abstract encoder boundary: one typed message -> one wire record. Lets the
 * publisher stay agnostic to the wire format.
 *
 * IMPROVEMENT: return a lab::fixed_string / span once a benchmark shows
 * the std::string allocation matters on the hot path.
 */

namespace market_data {

class encoder
{
public:
  virtual ~encoder() = default;

  virtual std::string encode(const message& msg) const = 0;
};

} // namespace market_data

#endif /* MARKET_DATA_ENCODER_HPP */
