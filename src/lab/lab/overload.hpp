#ifndef LAB_OVERLOAD_HPP
#define LAB_OVERLOAD_HPP

namespace lab {

template <typename... Ts>
struct overload : Ts...
{
  using Ts::operator()...;
};

template <typename... Ts>
overload(Ts...) -> overload<Ts...>;

} // namespace lab

#endif /* LAB_OVERLOAD_HPP */
