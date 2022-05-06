#include <iol/execution/run_loop.hpp>

#include <utility>

namespace iol::execution
{

namespace run_loop__
{

run_loop::run_loop() noexcept
  : head_(nullptr), tail_(&head_), n_work_{0}, state_{state_t::running}, mut_{}, cv_{}
{}

run_loop::~run_loop()
{
  [[unlikely]] if (n_work_ != 0 || state_ == state_t::running) std::terminate();
}

void run_loop::run()
{
  while (auto* op = pop_front())
    op->execute(op);
}

void run_loop::finish()
{
  {
    std::scoped_lock<std::mutex> lock{mut_};
    state_ = state_t::finishing;
  }
  cv_.notify_all();
}

void run_loop::push_back(opstate_base* op)
{
  {
    std::scoped_lock<std::mutex> lock{mut_};
    ++n_work_;
    *tail_ = op;
    tail_ = &op->next_;
  }
  cv_.notify_one();
}

opstate_base* run_loop::pop_front()
{
  std::unique_lock<std::mutex> lock{mut_};

  cv_.wait(lock, [&, this] { return (n_work_ > 0 || state_ == state_t::finishing); });
  // If n_work is 0 we know the state is also finishing
  if (n_work_ == 0)
    return nullptr;

  --n_work_;
  auto head = std::exchange(head_, head_->next_);
  tail_ = head_ ? tail_ : &head_;
  return head;
}

}  // namespace run_loop__

}  // namespace iol::execution
