#include "catch2/catch_test_macros.hpp"
#include "lab/defaulted_field.hpp"
#include "lab/fixed_string.hpp"
#include "lab/fmt.hpp"
#include "lab/json.hpp"
#include "lab/strong_type.hpp"

#include <string>
#include <vector>

namespace {

using venue_code = lab::strong_type<lab::fixed_string<16>, struct VenueCodeTag>;

struct gateway_config
{
  LAB_DEFAULTED_FIELD(int, timeout_ms, 5000);
  LAB_DEFAULTED_FIELD(bool, enabled, true);
  LAB_DEFAULTED_FIELD(venue_code, default_venue, "XNYS");
  LAB_DEFAULTED_FIELD(std::vector<std::string>, symbols);

  std::string session_name;

  bool operator==(const gateway_config&) const = default;
};

void to_json(nlohmann::json& json_object, const gateway_config& config)
{
  lab::json::write_field(json_object, "timeout_ms", config.timeout_ms);
  lab::json::write_field(json_object, "enabled", config.enabled);
  lab::json::write_field(json_object, "default_venue", config.default_venue);
  lab::json::write_field(json_object, "symbols", config.symbols);
  lab::json::write_field(json_object, "session_name", config.session_name);
}

void from_json(const nlohmann::json& json_object, gateway_config& config)
{
  lab::json::read_field(json_object, "timeout_ms", config.timeout_ms);
  lab::json::read_field(json_object, "enabled", config.enabled);
  lab::json::read_field(json_object, "default_venue", config.default_venue);
  lab::json::read_field(json_object, "symbols", config.symbols);
  lab::json::read_field(json_object, "session_name", config.session_name);
}

} // namespace

TEST_CASE("lab defaulted_field starts with its holder value", "[lab][defaulted_field]")
{
  gateway_config config{
    .session_name = "primary",
  };

  CHECK(config.timeout_ms == 5000);
  CHECK(config.enabled == true);
  CHECK(config.default_venue.value.get() == "XNYS");
  CHECK(config.symbols.value.empty());
}

TEST_CASE("lab defaulted_field behaves like the wrapped value", "[lab][defaulted_field]")
{
  gateway_config config{
    .session_name = "primary",
  };

  int timeout_ms = config.timeout_ms;
  CHECK(timeout_ms == 5000);

  config.timeout_ms = 250;
  config.symbols = std::vector<std::string>{"IBM", "MSFT"};

  CHECK(config.timeout_ms == 250);
  CHECK(250 == config.timeout_ms);
  CHECK(config.timeout_ms < 500);

  std::vector<std::string> symbols;
  for (const auto& symbol : config.symbols) {
    symbols.push_back(symbol);
  }
  CHECK(symbols == std::vector<std::string>{"IBM", "MSFT"});

  nlohmann::json serialized_timeout = config.timeout_ms;
  auto round_trip_timeout = serialized_timeout.get<decltype(config.timeout_ms)>();
  CHECK(round_trip_timeout == config.timeout_ms);

  CHECK(fmt::format("{}", config.timeout_ms) == "250");
}

TEST_CASE("lab json reads missing defaulted fields from holder defaults", "[lab][defaulted_field][json]")
{
  nlohmann::json json_object{
    {"session_name", "primary"},
  };

  auto config = json_object.get<gateway_config>();

  CHECK(config.timeout_ms == 5000);
  CHECK(config.enabled == true);
  CHECK(config.default_venue.value.get() == "XNYS");
  CHECK(config.symbols.value.empty());
  CHECK(config.session_name == "primary");
}

TEST_CASE("lab json reads and writes explicit defaulted field values", "[lab][defaulted_field][json]")
{
  gateway_config config{
    .session_name = "primary",
  };
  config.timeout_ms = 1000;
  config.enabled = false;
  config.default_venue = venue_code{"XNAS"};
  config.symbols = std::vector<std::string>{"NVDA"};

  nlohmann::json json_object = config;
  CHECK(json_object["timeout_ms"] == 1000);
  CHECK(json_object["enabled"] == false);
  CHECK(json_object["default_venue"] == "XNAS");
  CHECK(json_object["symbols"] == std::vector<std::string>{"NVDA"});
  CHECK(json_object["session_name"] == "primary");

  auto round_trip = json_object.get<gateway_config>();
  CHECK(round_trip == config);
}
