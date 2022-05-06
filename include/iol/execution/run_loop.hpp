#ifndef IOL_EXECUTION_RUN_LOOP_HPP
#define IOL_EXECUTION_RUN_LOOP_HPP

#include <iol/detail/config.hpp>

#include <iol/execution/receiver.hpp>
#include <iol/execution/start.hpp>
#include <iol/execution/connect.hpp>
#include <iol/execution/completion_signatures.hpp>
#include <iol/execution/scheduler.hpp>

#include <mutex>
#include <condition_variable>

namespace iol::execution
{

namespace run_loop_impl
{

class run_loop;

struct opstate_base;

using fn_type = void (*)(opstate_base*) noexcept;

struct opstate_base
{
  fn_type       execute;
  opstate_base* next_;
};

template <receiver_of R>
struct op_state : opstate_base
{

  template<typename Receiver>
  constexpr op_state(Receiver&& receiver, run_loop* loop) : opstate_base{{[](opstate_base* base) noexcept {
      auto& self = *static_cast<op_state*>(base);
      try {
          execution::set_value((R&&)self.r_);
      } catch (...) {
          execution::set_error((R&&)self.r_, std::current_exception());
      }
  }}, {nullptr}}, r_{(Receiver &&) receiver}, loop_{loop}
  {}

 private:

  [[no_unique_address]] R r_;

  run_loop* loop_;

  void start() noexcept;

  friend void tag_invoke(start_t, op_state<R>& self) noexcept { self.start(); }
};

class run_loop_scheduler;

class run_loop_sender
  : public completion_signatures<set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>
{

 public:

  constexpr run_loop_sender(run_loop* loop) : loop_{loop} {}

  template <receiver_of R>
  friend constexpr op_state<std::decay_t<R>> tag_invoke(
      connect_t, run_loop_sender const& self,
      R&& r) noexcept(std::is_nothrow_constructible_v<std::decay_t<R>, R>)
  {
    return {(R &&) r, self.loop_};
  }

  template <typename CPO>
  friend constexpr run_loop_scheduler tag_invoke(
      get_completion_scheduler_t<CPO>, run_loop_sender const& s) noexcept;

 private:

  run_loop* loop_;
};

class run_loop_scheduler
{

 public:

  constexpr run_loop_scheduler(run_loop* loop) : loop_{loop} {}

  friend constexpr bool operator==(
      run_loop_scheduler const& lhs, run_loop_scheduler const& rhs) noexcept
  {
    return lhs.loop_ == rhs.loop_;
  }

  friend constexpr bool operator!=(
      run_loop_scheduler const& lhs, run_loop_scheduler const& rhs) noexcept
  {
    return lhs.loop_ != rhs.loop_;
  }

  friend constexpr run_loop_sender tag_invoke(schedule_t, run_loop_scheduler const& sched) noexcept
  {
    return {sched.loop_};
  }

 private:

  run_loop* loop_;
};

template <typename CPO>
constexpr run_loop_scheduler tag_invoke(
    get_completion_scheduler_t<CPO>, run_loop_sender const& s) noexcept
{
  return {s.loop_};
}

class run_loop
{

  template <receiver_of>
  friend struct op_state;

  using work_count_t = std::size_t;

  enum class state_t { /* starting, */ running, finishing };

  friend constexpr forward_progress_guarantee tag_invoke(
      get_forward_progress_guarantee_t, run_loop const&) noexcept
  {
    return forward_progress_guarantee::parallel;
  }

 public:

  run_loop() noexcept;

  ~run_loop();

  run_loop(run_loop&&) = delete;

  run_loop_scheduler get_scheduler() { return {this}; }

  void run();

  void finish();

 private:

  void push_back(opstate_base* op);

  opstate_base* pop_front();

  opstate_base *head_, **tail_;
  work_count_t  n_work_;
  state_t       state_;

  std::mutex              mut_;
  std::condition_variable cv_;
};

template <receiver_of R>
void op_state<R>::start() noexcept
{
  // This is non-throwing atm
  // try {
  loop_->push_back(this);
  // } catch (...) {
  //   execution::set_error(std::move(self), std::current_exception());
  // }
}

}  // namespace run_loop_impl

using run_loop_impl::run_loop;

}  // namespace iol::execution

#endif  // IOL_EXECUTION_RUN_LOOP_HPP
