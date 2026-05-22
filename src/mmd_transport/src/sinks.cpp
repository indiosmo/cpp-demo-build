#include "mmd_transport/sinks.hpp"

namespace mmd_transport {

void capture_sink::publish(std::string_view encoded_record)
{
  records_.emplace_back(encoded_record);
}

const std::vector<std::string>& capture_sink::records() const
{
  return records_;
}

} // namespace mmd_transport
