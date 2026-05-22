#ifndef MMDFIX_CONVERSIONS_HPP
#define MMDFIX_CONVERSIONS_HPP

#include "mmd/messages.hpp"
#include "mmdfix/messages.hpp"

namespace mmdfix {

[[nodiscard]] market_data_incremental_refresh to_fix(const mmd::mbo_book_update& event);
[[nodiscard]] trade_capture_report to_fix(const mmd::trade& event);

} // namespace mmdfix

#endif /* MMDFIX_CONVERSIONS_HPP */
