#include <iol/detail/operation_queue.hpp>
#include <utility>

namespace iol::detail
{

operation_queue::operation_queue() noexcept : head_{}, tail_{&head_} {};

operation_queue::operation_queue(operation_queue&& other) noexcept
  : head_{std::move(other.head_)}, tail_{head_ ? other.tail_ : &head_}
{
  other.tail_ = &other.head_;
}

operation_queue& operation_queue::operator=(operation_queue&& other) noexcept
{
  if (this != &other) {
    operation_queue tmp{std::move(other)};
    swap(tmp);
  }
  return *this;
}

void operation_queue::swap(operation_queue& other) noexcept
{
  using std::swap;
  swap(head_, other.head_);
  auto* temp = tail_;
  tail_ = head_ ? other.tail_ : &head_;
  other.tail_ = other.head_ ? temp : &other.head_;
}

bool operation_queue::empty() const noexcept
{
  return &head_ == tail_;
}

void operation_queue::enqueue(operation_ptr&& operation) noexcept
{
  *tail_ = std::move(operation);
  tail_ = &((*tail_)->next);
}

void operation_queue::enqueue(operation_queue&& queue) noexcept
{
  if (queue.empty())
    return;
  *tail_ = std::move(queue.head_);
  tail_ = std::exchange(queue.tail_, &queue.head_);
}

operation_ptr operation_queue::deque() noexcept
{
  IOL_ASSERT(head_);
  operation_ptr ret = std::exchange(head_, std::move(head_->next));
  tail_ = head_ ? tail_ : &head_;
  return ret;
}

}  // namespace iol::detail
