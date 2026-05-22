#ifndef MMD_TRANSPORT_SINKS_HPP
#define MMD_TRANSPORT_SINKS_HPP

#include <string>
#include <string_view>
#include <vector>

namespace mmd_transport {

class sink
{
public:
  virtual ~sink() = default;

  virtual void publish(std::string_view encoded_record) = 0;
};

class capture_sink final : public sink
{
public:
  void publish(std::string_view encoded_record) override;

  [[nodiscard]] const std::vector<std::string>& records() const;

private:
  std::vector<std::string> records_;
};

} // namespace mmd_transport

#endif /* MMD_TRANSPORT_SINKS_HPP */
