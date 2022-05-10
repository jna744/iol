#include <iostream>
#include <string_view>
#include <chrono>

#include <iol/execution/run_loop.hpp>
#include <iol/execution/sync_wait.hpp>
#include <iol/execution/general_queries.hpp>
#include <iol/execution/then.hpp>
#include <iol/execution/sender_factories.hpp>
#include <iol/execution/sender.hpp>
#include <iol/meta.hpp>

template <typename T>
constexpr auto type_name() -> std::string_view
{
  constexpr auto prefix = std::string_view{"[with T = "};
  constexpr auto suffix = std::string_view{";"};
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};

  constexpr auto start = function.find(prefix) + prefix.size();
  constexpr auto end = function.rfind(suffix);

  static_assert(start < end);

  constexpr auto result = function.substr(start, (end - start));

  return result;
}

#define PRINT(T) std::cout << type_name<T>() << std::endl

struct dummy_receiver
{

  template <typename... Ts>
  friend void tag_invoke(iol::execution::set_value_t, dummy_receiver&& r, int) noexcept
  {
    std::cout << "hello from dummy" << std::endl;
  }

  template <typename Error>
  friend void tag_invoke(iol::execution::set_error_t, dummy_receiver&& r, Error&& e) noexcept
  {}

  friend void tag_invoke(iol::execution::set_stopped_t, dummy_receiver&& r) noexcept {}
};

template <typename>
struct S
{};

template <typename>
struct _S2
{
  struct type
  {};
};

template <typename T>
using S2 = typename _S2<T>::type;

namespace N
{
int f(auto)
{
  return 0;
};
struct A
{};
};  // namespace N

struct Q
{
  int i;
};

int main(int argc, char* argv[])
{

  using namespace iol::execution;

  auto v = get_scheduler() | then(
                                 [](auto sched)
                                 {
                                   using T = decltype(sched);
                                   PRINT(T);
                                 });

  auto u = just(1, 2, 3, 4) | then([](auto...) { return 5; });

  static_assert(sender<decltype(v)>);

  sync_wait(std::move(v));

  return 0;
}
