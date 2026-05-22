#ifndef MOR_INTERFACES_HPP
#define MOR_INTERFACES_HPP

#include "lab/inplace_function.hpp"
#include "mor/messages.hpp"

namespace mor {

using request_callback = lab::inplace_function<void(const request&), 128>;
using event_callback = lab::inplace_function<void(const event&), 128>;

struct source
{
  request_callback on_request;
};

class sink
{
public:
  virtual ~sink() = default;

  virtual void send(const request& request) = 0;
};

struct pipeline_stage
{
  request_callback on_request;
  event_callback on_event;
};

inline void wire(source& source_stage, sink& target)
{
  source_stage.on_request = [&target](const request& routed_request) { target.send(routed_request); };
}

inline void wire(pipeline_stage& upstream, pipeline_stage& downstream)
{
  upstream.on_request = [&downstream](const request& routed_request) { downstream.on_request(routed_request); };
  downstream.on_event = [&upstream](const event& routed_event) { upstream.on_event(routed_event); };
}

} // namespace mor

#endif /* MOR_INTERFACES_HPP */
