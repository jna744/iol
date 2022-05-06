#ifndef IOL_DETAIL_OPERATION_QUEUE_HPP
#define IOL_DETAIL_OPERATION_QUEUE_HPP

#include <iol/detail/config.hpp>
#include <iol/detail/operation_base.hpp>

namespace iol::detail
{

class operation_queue
{

public:

  operation_queue() noexcept;

  operation_queue(operation_queue&& other) noexcept;

  operation_queue& operator=(operation_queue&& other) noexcept;

  void swap(operation_queue& other) noexcept;

  bool empty() const noexcept;

  void enqueue(operation_ptr&& operation) noexcept;

  void enqueue(operation_queue&& queue) noexcept;

  operation_ptr deque() noexcept;

private:

  operation_ptr  head_;
  operation_ptr* tail_;
};

inline void swap(operation_queue& lhs, operation_queue& rhs) noexcept
{
  lhs.swap(rhs);
}

}  // namespace iol::detail

#endif  // IOL_DETAIL_OPERATION_QUEUE_HPP
