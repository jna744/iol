#ifndef IOL_DETAIL_OPERATION_BASE_HPP
#define IOL_DETAIL_OPERATION_BASE_HPP

#include <memory>

namespace iol::detail
{

struct operation_base;

struct operation_destroyer {
  template <typename Task>
  constexpr void operator()(Task *task) const noexcept
  {
    task->invoke(nullptr, task);
  }
};

using operation_ptr = std::unique_ptr<operation_base, operation_destroyer>;

struct operation_base {
  using func = void (*)(void *, operation_base *);
  func          invoke;
  operation_ptr next;
};

}  // namespace iol::detail

#endif  // IOL_DETAIL_OPERATION_BASE_HPP
