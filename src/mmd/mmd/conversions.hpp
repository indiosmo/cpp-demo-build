#ifndef MMD_CONVERSIONS_HPP
#define MMD_CONVERSIONS_HPP

#include "lab/variant.hpp"
#include "market_data/messages.hpp"
#include "mmd/messages.hpp"

namespace mmd {

[[nodiscard]] security_definition to_mmd(const market_data::security_definition& message);
[[nodiscard]] security_status to_mmd(const market_data::security_status& message);
[[nodiscard]] execution_summary to_mmd(const market_data::execution_summary& message);
[[nodiscard]] trade to_mmd(const market_data::trade& message);
[[nodiscard]] mbo_book_update to_mmd(const market_data::mbo_book_update& message);
[[nodiscard]] message to_mmd(const market_data::message& message);

[[nodiscard]] market_data::security_definition to_market_data(const security_definition& message);
[[nodiscard]] market_data::security_status to_market_data(const security_status& message);
[[nodiscard]] market_data::execution_summary to_market_data(const execution_summary& message);
[[nodiscard]] market_data::trade to_market_data(const trade& message);
[[nodiscard]] market_data::mbo_book_update to_market_data(const mbo_book_update& message);
[[nodiscard]] market_data::message to_market_data(const message& message);

} // namespace mmd

#endif /* MMD_CONVERSIONS_HPP */
