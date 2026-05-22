#include "catch2/catch_test_macros.hpp"
#include "lab/json.hpp"
#include "lab/strong_type.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct order_snapshot
{
  std::string symbol;
  std::uint64_t bid_quantity{};
  std::uint64_t ask_quantity{};
  std::optional<std::string> liquidity;

  bool operator==(const order_snapshot&) const = default;
};

void to_json(nlohmann::json& json_object, const order_snapshot& snapshot)
{
  lab::json::write_field(json_object, "symbol", snapshot.symbol);
  lab::json::write_field(json_object, "bid_quantity", snapshot.bid_quantity);
  lab::json::write_field(json_object, "ask_quantity", snapshot.ask_quantity);
  lab::json::write_field(json_object, "liquidity", snapshot.liquidity);
}

void from_json(const nlohmann::json& json_object, order_snapshot& snapshot)
{
  lab::json::read_field(json_object, "symbol", snapshot.symbol);
  lab::json::read_field(json_object, "bid_quantity", snapshot.bid_quantity);
  lab::json::read_field(json_object, "ask_quantity", snapshot.ask_quantity);
  lab::json::read_field(json_object, "liquidity", snapshot.liquidity);
}

using venue_code = lab::strong_type<lab::fixed_string<8>, struct VenueCodeTag>;

struct venue_settings
{
  venue_code venue;
  lab::fixed_string<16> symbol;
  std::chrono::milliseconds latency{};

  bool operator==(const venue_settings&) const = default;
};

void to_json(nlohmann::json& json_object, const venue_settings& settings)
{
  lab::json::write_field(json_object, "venue", settings.venue);
  lab::json::write_field(json_object, "symbol", settings.symbol);
  lab::json::write_field(json_object, "latency", settings.latency);
}

void from_json(const nlohmann::json& json_object, venue_settings& settings)
{
  lab::json::read_field(json_object, "venue", settings.venue);
  lab::json::read_field(json_object, "symbol", settings.symbol);
  lab::json::read_field(json_object, "latency", settings.latency);
}

struct jsonl_row
{
  int id{};
  std::string symbol;

  bool operator==(const jsonl_row&) const = default;
};

void from_json(const nlohmann::json& json_object, jsonl_row& row)
{
  lab::json::read_field(json_object, "id", row.id);
  lab::json::read_field(json_object, "symbol", row.symbol);
}

struct described_config
{
  std::string venue;
  LAB_DEFAULTED_FIELD(int, max_connections, 8);
  std::optional<std::string> log_file;

  bool operator==(const described_config&) const = default;
};

LAB_AUTO_JSON(described_config, venue, max_connections, log_file)

struct temp_directory
{
  temp_directory()
    : path{scratch_root() / "matching-engine-lab-json-test"}
  {
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
  }

  ~temp_directory()
  {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }

  std::filesystem::path path;

private:
  static std::filesystem::path scratch_root()
  {
    if (const char* temp_dir = std::getenv("TMPDIR"); temp_dir != nullptr) {
      return temp_dir;
    }
    return std::filesystem::current_path();
  }
};

} // namespace

TEST_CASE("lab json serializes aggregate fields", "[lab][json]")
{
  order_snapshot snapshot{
    .symbol = "IBM",
    .bid_quantity = 100,
    .ask_quantity = 125,
    .liquidity = "maker",
  };

  nlohmann::json serialized = snapshot;
  CHECK(serialized["symbol"] == "IBM");
  CHECK(serialized["bid_quantity"] == 100);
  CHECK(serialized["ask_quantity"] == 125);
  CHECK(serialized["liquidity"] == "maker");

  auto round_trip = serialized.get<order_snapshot>();
  CHECK(round_trip == snapshot);
}

TEST_CASE("lab json leaves missing optionals empty", "[lab][json]")
{
  nlohmann::json serialized{
    {"symbol", "IBM"},
    {"bid_quantity", 100},
    {"ask_quantity", 125},
  };

  auto snapshot = serialized.get<order_snapshot>();
  CHECK(snapshot.symbol == "IBM");
  CHECK(snapshot.bid_quantity == 100);
  CHECK(snapshot.ask_quantity == 125);
  CHECK_FALSE(snapshot.liquidity.has_value());

  nlohmann::json round_trip = snapshot;
  CHECK_FALSE(round_trip.contains("liquidity"));
}

TEST_CASE("lab json serializes fixed strings, strong types, and chrono durations", "[lab][json]")
{
  venue_settings settings{
    .venue = venue_code{lab::fixed_string<8>{"XNYS"}},
    .symbol = lab::fixed_string<16>{"IBM"},
    .latency = std::chrono::milliseconds{250},
  };

  nlohmann::json serialized = settings;
  CHECK(serialized["venue"] == "XNYS");
  CHECK(serialized["symbol"] == "IBM");
  CHECK(serialized["latency"] == 250);

  auto round_trip = serialized.get<venue_settings>();
  CHECK(round_trip == settings);
}

TEST_CASE("lab json serializes fixed string keyed maps as objects", "[lab][json]")
{
  using symbol = lab::fixed_string<8>;
  std::unordered_map<symbol, int> quantities;
  quantities.emplace(symbol{"IBM"}, 100);
  quantities.emplace(symbol{"MSFT"}, 200);

  nlohmann::json serialized = quantities;
  CHECK(serialized["IBM"] == 100);
  CHECK(serialized["MSFT"] == 200);

  auto round_trip = serialized.get<std::unordered_map<symbol, int>>();
  CHECK(round_trip.at(symbol{"IBM"}) == 100);
  CHECK(round_trip.at(symbol{"MSFT"}) == 200);
}

TEST_CASE("lab json try_parse returns typed values", "[lab][json]")
{
  auto result = lab::json::try_parse<order_snapshot>(R"({"symbol":"IBM","bid_quantity":100,"ask_quantity":125})");
  REQUIRE(result);

  CHECK(result.value().symbol == "IBM");
  CHECK(result.value().bid_quantity == 100);
  CHECK(result.value().ask_quantity == 125);
}

TEST_CASE("lab json try_parse reports invalid input", "[lab][json]")
{
  auto result = lab::json::try_parse<order_snapshot>("not-json");
  CHECK_FALSE(result);
}

TEST_CASE("lab json reads jsonl files", "[lab][json]")
{
  temp_directory temp;
  auto file_path = temp.path / "rows.jsonl";
  {
    std::ofstream output{file_path};
    REQUIRE(output.is_open());
    output << R"({"id":1,"symbol":"IBM"})" << '\n';
    output << R"({"id":2,"symbol":"MSFT"})" << '\n';
  }

  auto result = lab::json::read_jsonl<jsonl_row>(file_path.string());
  REQUIRE(result);

  const auto& rows = result.value();
  REQUIRE(rows.size() == 2);
  CHECK(rows[0] == jsonl_row{.id = 1, .symbol = "IBM"});
  CHECK(rows[1] == jsonl_row{.id = 2, .symbol = "MSFT"});
}

TEST_CASE("lab json reports invalid jsonl lines", "[lab][json]")
{
  temp_directory temp;
  auto file_path = temp.path / "rows.jsonl";
  {
    std::ofstream output{file_path};
    REQUIRE(output.is_open());
    output << R"({"id":1,"symbol":"IBM"})" << '\n';
    output << "not-json" << '\n';
  }

  auto result = lab::json::read_jsonl<jsonl_row>(file_path.string());
  CHECK_FALSE(result);
}

TEST_CASE("lab json auto binding reads described config objects", "[lab][json]")
{
  const nlohmann::json serialized{
    {"venue", "B3"},
  };

  const auto config = serialized.get<described_config>();

  CHECK(config.venue == "B3");
  CHECK(config.max_connections == 8);
  CHECK_FALSE(config.log_file.has_value());
}

TEST_CASE("lab json auto binding rejects unknown fields", "[lab][json]")
{
  const nlohmann::json serialized{
    {"venue", "B3"},
    {"unexpected", true},
  };

  CHECK_THROWS_AS(serialized.get<described_config>(), std::runtime_error);
}
