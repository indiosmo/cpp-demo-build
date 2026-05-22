#ifndef KRAKEN_OVERLOAD_HPP
#define KRAKEN_OVERLOAD_HPP

namespace kraken {

template <typename... Ts>
struct overload : Ts...
{
  using Ts::operator()...;
};

template <typename... Ts>
overload(Ts...) -> overload<Ts...>;

} // namespace kraken

#endif /* KRAKEN_OVERLOAD_HPP */
