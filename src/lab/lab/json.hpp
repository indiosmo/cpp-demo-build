#ifndef LAB_JSON_HPP
#define LAB_JSON_HPP

#include "lab/defaulted_field.hpp"
#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/fixed_string.hpp"
#include "lab/fmt.hpp"
#include "lab/result.hpp"
#include "lab/strong_type.hpp"
#include "nlohmann/json.hpp"

#include <chrono>
#include <concepts>
#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lab::json {

using value = nlohmann::json;

template <typename T>
concept Serializable = requires (const T& object, nlohmann::json& json_object, const nlohmann::json& const_json_object) {
  { to_json(json_object, object) } -> std::same_as<void>;
  { from_json(const_json_object, std::declval<T&>()) } -> std::same_as<void>;
};

template <typename T>
std::string dump(T&& object)
{
  return nlohmann::json(std::forward<T>(object)).dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

inline std::string dump(const nlohmann::json& object)
{
  return object.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

inline lab::result<nlohmann::json> try_parse(std::string_view json_text)
{
  try {
    return nlohmann::json::parse(json_text.begin(), json_text.end());
  } catch (const std::exception& ex) {
    return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("invalid json: {}", ex.what()));
  }
}

template <typename T>
lab::result<T> try_cast(const nlohmann::json& json_object)
{
  try {
    return json_object.template get<T>();
  } catch (const std::exception& ex) {
    return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("invalid json value: {}", ex.what()));
  }
}

template <typename T>
lab::result<T> try_parse(std::string_view json_text)
{
  BOOST_LEAF_ASSIGN(auto json_object, try_parse(json_text));
  return try_cast<T>(json_object);
}

template <typename T>
lab::result<T> read_from_file(const std::string& filename, bool ignore_comments = false)
{
  std::ifstream input{filename};
  if (!input.is_open()) {
    return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("file not found: {}", filename));
  }

  try {
    auto json_object = nlohmann::json::parse(input, nullptr, true, ignore_comments);
    return json_object.template get<T>();
  } catch (const std::exception& ex) {
    return lab::make_leaf_error(
      lab::error_code::invalid_argument, fmt::format("invalid json file '{}': {}", filename, ex.what()));
  }
}

template <typename T>
lab::result<std::vector<T>> read_jsonl(const std::string& filename)
{
  std::ifstream input{filename};
  if (!input.is_open()) {
    return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("file not found: {}", filename));
  }

  std::vector<T> values;
  std::string line;

  while (std::getline(input, line)) {
    BOOST_LEAF_ASSIGN(auto value, try_parse<T>(line));
    values.push_back(std::move(value));
  }

  if (input.bad()) {
    return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("failed to read file: {}", filename));
  }

  return values;
}

template <typename T>
void read_field(const nlohmann::json& json_object, std::string_view key, T& field)
{
  json_object.at(std::string{key}).get_to(field);
}

template <typename T>
void write_field(nlohmann::json& json_object, std::string_view key, const T& field)
{
  json_object[std::string{key}] = field;
}

template <DefaultedField T>
void read_field(const nlohmann::json& json_object, std::string_view key, T& field)
{
  if (auto it = json_object.find(std::string{key}); it != json_object.end()) {
    it->get_to(field.value);
  }
}

template <DefaultedField T>
void write_field(nlohmann::json& json_object, std::string_view key, const T& field)
{
  json_object[std::string{key}] = field.value;
}

template <typename T>
void read_field(const nlohmann::json& json_object, std::string_view key, std::optional<T>& field)
{
  if (auto it = json_object.find(std::string{key}); it != json_object.end()) {
    it->get_to(field);
  }
}

template <typename T>
void write_field(nlohmann::json& json_object, std::string_view key, const std::optional<T>& field)
{
  if (field.has_value()) {
    json_object[std::string{key}] = *field;
  }
}

} // namespace lab::json

namespace lab {

template <std::uint8_t N, fixed_string_truncation_policy Policy>
void to_json(nlohmann::json& to, const fixed_string<N, Policy>& from)
{
  to = from.to_string();
}

template <std::uint8_t N>
void from_json(const nlohmann::json& from, fixed_string<N, fixed_string_truncation_policy::strict>& to)
{
  auto parsed = fixed_string<N, fixed_string_truncation_policy::strict>::from(from.get<std::string>());
  if (!parsed) {
    throw std::runtime_error{"fixed_string out of bounds"};
  }
  to = *parsed;
}

template <std::uint8_t N>
void from_json(const nlohmann::json& from, fixed_string<N, fixed_string_truncation_policy::auto_truncate>& to)
{
  to = fixed_string<N, fixed_string_truncation_policy::auto_truncate>{from.get<std::string>()};
}

} // namespace lab

namespace nlohmann {

template <typename T>
struct adl_serializer<std::optional<T>>
{
  static void to_json(json& to, const std::optional<T>& from)
  {
    if (from.has_value()) {
      to = *from;
    } else {
      to = nullptr;
    }
  }

  static void from_json(const json& from, std::optional<T>& to)
  {
    if (from.is_null()) {
      to = std::nullopt;
    } else {
      to = from.get<T>();
    }
  }
};

template <typename T, typename Parameter, template <typename> class... Skills>
struct adl_serializer<lab::strong_type<T, Parameter, Skills...>>
{
  using value_type = lab::strong_type<T, Parameter, Skills...>;

  static void to_json(json& to, const value_type& from)
  {
    to = from.get();
  }

  static void from_json(const json& from, value_type& to)
  {
    to = value_type{from.get<typename value_type::UnderlyingType>()};
  }
};

template <typename T, typename DefaultHolder>
struct adl_serializer<lab::defaulted_field<T, DefaultHolder>>
{
  using value_type = lab::defaulted_field<T, DefaultHolder>;

  static void to_json(json& to, const value_type& from)
  {
    to = from.value;
  }

  static void from_json(const json& from, value_type& to)
  {
    from.get_to(to.value);
  }
};

template <typename Rep, typename Ratio>
struct adl_serializer<std::chrono::duration<Rep, Ratio>>
{
  static void to_json(json& to, const std::chrono::duration<Rep, Ratio>& from)
  {
    to = from.count();
  }

  static void from_json(const json& from, std::chrono::duration<Rep, Ratio>& to)
  {
    to = std::chrono::duration<Rep, Ratio>{from.get<Rep>()};
  }
};

template <typename T, std::uint8_t N>
struct adl_serializer<std::unordered_map<lab::fixed_string<N>, T>>
{
  static void to_json(json& to, const std::unordered_map<lab::fixed_string<N>, T>& from)
  {
    to = nlohmann::json::object();
    for (const auto& [key, value] : from) {
      to[key.to_string()] = value;
    }
  }

  static void from_json(const json& from, std::unordered_map<lab::fixed_string<N>, T>& to)
  {
    for (const auto& [key, value] : from.items()) {
      auto parsed = lab::fixed_string<N>::from(key);
      if (!parsed) {
        throw std::runtime_error{"fixed_string out of bounds"};
      }
      to[*parsed] = value.template get<T>();
    }
  }
};

} // namespace nlohmann

template <>
struct fmt::formatter<nlohmann::json>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const nlohmann::json& value, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "{0}", value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
  }
};

#endif /* LAB_JSON_HPP */
