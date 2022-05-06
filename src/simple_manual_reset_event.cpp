#include <iol/detail/simple_manual_reset_event.hpp>

#if defined(linux)

#include <linux/futex.h> /* Definition of FUTEX_* constants */
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>

#include <numeric>

namespace
{

namespace local
{

inline long futex(
    uint32_t* uaddr, int futex_op, uint32_t val, timespec const* timeout, uint32_t* uaddr2,
    uint32_t val3)
{
  return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

}  // namespace local

}  // namespace

#endif

namespace iol::detail
{

simple_manual_reset_event::simple_manual_reset_event() noexcept
  :
#if !defined(linux)
    mut_{},
    cv_{},
#endif
    state_{0}
{}

void simple_manual_reset_event::set()
{
#if !defined(linux)
  std::scoped_lock<std::mutex> lock{mut_};
  state_ = true;
  cv_.notify_all();
#else
  state_.store(1, std::memory_order_release);
  [[maybe_unused]] auto n_awoken = local::futex(
      (uint32_t*)&state_, FUTEX_WAKE_PRIVATE,
      // How many to wake up
      std::numeric_limits<int>::max(), nullptr, nullptr, 0);
  IOL_ASSERT(n_awoken != -1);
#endif
}

void simple_manual_reset_event::reset()
{
#if !defined(linux)
  std::scoped_lock<std::mutex> lock{mut_};
  state_.store(true, std::memory_order_relaxed);
#else
  state_.store(0, std::memory_order_relaxed);
#endif
}

void simple_manual_reset_event::wait() const
{
#if !defined(linux)
  std::unique_lock<std::mutex> lock{mut_};
  cv_.wait(lock, [this] { return state_.load(std::memory_order_relaxed); });
#endif
  int old_state = state_.load(std::memory_order_acquire);
  while (!old_state) {
    if (local::futex((uint32_t*)&state_, FUTEX_WAIT_PRIVATE, old_state, nullptr, nullptr, 0) ==
        -1) {
      // state changed before we could wait
      if (errno == EAGAIN)
        return;
    }
    old_state = state_.load(std::memory_order_acquire);
  }
}

bool simple_manual_reset_event::is_set() const
{
#if !defined(linux)
  return state_.load(std::memory_order_relaxed);
#else
  return state_.load(std::memory_order_relaxed);
#endif
}

}  // namespace iol::detail
