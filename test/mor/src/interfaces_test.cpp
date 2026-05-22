#include "catch2/catch_test_macros.hpp"
#include "mor/interfaces.hpp"

#include <optional>

namespace {

class capture_sink final : public mor::sink
{
public:
  void send(const mor::request& request) override
  {
    captured_request = request;
  }

  std::optional<mor::request> captured_request;
};

} // namespace

TEST_CASE("mor interfaces - source wires requests into a sink", "[mor][unit]")
{
  mor::source source;
  capture_sink sink;

  mor::wire(source, sink);
  source.on_request(mor::cancel_request{
    .client_id = mor::types::client_id{7},
    .cl_ord_id = mor::types::cl_ord_id{99},
    .orig_cl_ord_id = mor::types::orig_cl_ord_id{42},
  });

  REQUIRE(sink.captured_request.has_value());
  REQUIRE(std::holds_alternative<mor::cancel_request>(*sink.captured_request));
  CHECK(std::get<mor::cancel_request>(*sink.captured_request).orig_cl_ord_id == mor::types::orig_cl_ord_id{42});
}

TEST_CASE("mor interfaces - pipeline stages wire requests and events", "[mor][unit]")
{
  mor::pipeline_stage upstream;
  mor::pipeline_stage downstream;
  std::optional<mor::event> captured_event;
  std::optional<mor::request> captured_request;

  upstream.on_event = [&captured_event](const mor::event& event) { captured_event = event; };
  downstream.on_request = [&captured_request](const mor::request& request) { captured_request = request; };

  mor::wire(upstream, downstream);

  upstream.on_request(mor::flush_request{});
  downstream.on_event(mor::parser_reject{.raw_payload = "{}", .reason = "missing message_type"});

  REQUIRE(captured_request.has_value());
  CHECK(std::holds_alternative<mor::flush_request>(*captured_request));
  REQUIRE(captured_event.has_value());
  CHECK(std::holds_alternative<mor::parser_reject>(*captured_event));
}

