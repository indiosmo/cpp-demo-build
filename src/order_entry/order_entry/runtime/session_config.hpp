#ifndef ORDER_ENTRY_RUNTIME_SESSION_CONFIG_HPP
#define ORDER_ENTRY_RUNTIME_SESSION_CONFIG_HPP

#include "lab/network/types.hpp"
#include "lab/defaulted_field.hpp"
#include "lab/json.hpp"

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

/*
 * Configuration surface for order_entry::runtime::session. Backend
 * choice is a variant: a new UDP transport or decoder slots in as a new
 * alternative plus a matching setup overload in the runtime.
 */

namespace order_entry::runtime {

// Boost.Asio kernel-socket receiver. The wiring shell owns the io_context
// and drives its poll tick from the receive event loop, so the receiver
// does not need its own thread.
struct asio_udp_receiver_config
{
  lab::network::types::endpoint_config endpoint;
};

LAB_AUTO_JSON(asio_udp_receiver_config, endpoint)

// Solarflare ef_vi kernel-bypass receiver. The implementation is a stub that
// keeps the poll-driven composer branch buildable.
struct ef_vi_udp_receiver_config
{
  lab::network::types::endpoint_config endpoint;
};

LAB_AUTO_JSON(ef_vi_udp_receiver_config, endpoint)

using receiver_config = std::variant<asio_udp_receiver_config, ef_vi_udp_receiver_config>;

struct json_decoder_config
{
  LAB_DEFAULTED_FIELD(std::size_t, max_datagram_size, 65535);
};

LAB_AUTO_JSON(json_decoder_config, max_datagram_size)

using decoder_config = std::variant<json_decoder_config>;

struct session_config
{
  receiver_config receiver;
  decoder_config decoder{json_decoder_config{}};
};

BOOST_DESCRIBE_STRUCT(session_config, (), (receiver, decoder))

inline void to_json(nlohmann::json& json_object, const receiver_config& config)
{
  std::visit(
    [&](const auto& selection) {
      using selection_t = std::decay_t<decltype(selection)>;
      json_object = selection;
      if constexpr (std::is_same_v<selection_t, asio_udp_receiver_config>) {
        json_object["type"] = "asio_udp";
      } else if constexpr (std::is_same_v<selection_t, ef_vi_udp_receiver_config>) {
        json_object["type"] = "ef_vi_udp";
      }
    },
    config);
}

inline void from_json(const nlohmann::json& json_object, receiver_config& config)
{
  const auto type = lab::json::read_type(json_object);
  if (type == "asio_udp") {
    asio_udp_receiver_config receiver;
    lab::json::read_field(json_object, "endpoint", receiver.endpoint);
    config = std::move(receiver);
    return;
  }
  if (type == "ef_vi_udp") {
    ef_vi_udp_receiver_config receiver;
    lab::json::read_field(json_object, "endpoint", receiver.endpoint);
    config = std::move(receiver);
    return;
  }

  throw std::runtime_error{"unknown order-entry receiver type '" + type + "'"};
}

inline void to_json(nlohmann::json& json_object, const decoder_config& config)
{
  std::visit(
    [&](const auto& selection) {
      json_object = selection;
      json_object["type"] = "json";
    },
    config);
}

inline void from_json(const nlohmann::json& json_object, decoder_config& config)
{
  const auto type = lab::json::read_type(json_object);
  if (type == "json") {
    json_decoder_config decoder;
    if (json_object.contains("max_datagram_size")) {
      lab::json::read_field(json_object, "max_datagram_size", decoder.max_datagram_size);
    }
    config = decoder;
    return;
  }

  throw std::runtime_error{"unknown order-entry decoder type '" + type + "'"};
}

inline void to_json(nlohmann::json& json_object, const session_config& config)
{
  lab::json::write_described_object(json_object, config);
}

inline void from_json(const nlohmann::json& json_object, session_config& config)
{
  lab::json::read_field(json_object, "receiver", config.receiver);
  if (json_object.contains("decoder")) {
    lab::json::read_field(json_object, "decoder", config.decoder);
  }
}

} // namespace order_entry::runtime

#endif /* ORDER_ENTRY_RUNTIME_SESSION_CONFIG_HPP */
